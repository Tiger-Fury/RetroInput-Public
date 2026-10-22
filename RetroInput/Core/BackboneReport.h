#pragma once
#include <Windows.h>
#include <Xinput.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <array>
#include <cstdint>

namespace RetroInput::BackboneReports {
inline bool IsSupportedDevice(USHORT vendor, USHORT product) {
    return vendor == 0x358A && product == 0x0304;
}
inline bool IsSupportedReportBuffer(DWORD size) { return size >= 8 && size <= 1024; }
struct Field {
    USAGE usage = 0;
    USHORT link = 0, bits = 0, index = 0;
    LONG minimum = 0, maximum = 0;
    BYTE reportId = 0;
    USAGE page = 1;
};
struct Layout {
    // LX, LY, RX, RY, LT, RT, hat, in logical control order.
    std::array<Field,7> fields = {};
    USHORT buttonLink = 0;
    BYTE reportId = 0;
    bool valid = false;
    bool androidButtons = false;
};
inline Layout BuildLayout(const HIDP_VALUE_CAPS* values, unsigned count,
    const HIDP_BUTTON_CAPS* buttons, unsigned buttonCount) {
    // Accept a complete standard desktop gamepad state, never a vendor/touch report.
    bool accelerator=false, brake=false;
    for(unsigned i=0;i<count;++i) {
        const auto& c=values[i];
        if(c.UsagePage!=2)continue;
        const USAGE first=c.IsRange?c.Range.UsageMin:c.NotRange.Usage;
        const USAGE last=c.IsRange?c.Range.UsageMax:c.NotRange.Usage;
        accelerator|=first<=0xC4 && last>=0xC4;
        brake|=first<=0xC5 && last>=0xC5;
    }
    const bool simulation=accelerator && brake;
    // Actual 358A:0304 descriptor: X/Y and Z/Rz sticks, simulation-page triggers.
    // Do not infer semantic axis order from HID data indices: the live caps are
    // enumerated Rz, Z, Y, X and still describe the same logical controls.
    const USAGE usages[] = {0x30,0x31,static_cast<USAGE>(simulation?0x32:0x33),
        static_cast<USAGE>(simulation?0x35:0x34),static_cast<USAGE>(simulation?0xC5:0x32),
        static_cast<USAGE>(simulation?0xC4:0x35),0x39};
    const USAGE pages[] = {1,1,1,1,static_cast<USAGE>(simulation?2:1),static_cast<USAGE>(simulation?2:1),1};
    Layout result;
    for (unsigned target=0;target<7;++target) {
        for (unsigned i=0;i<count;++i) {
            const auto& c=values[i];
            const USAGE first=c.IsRange?c.Range.UsageMin:c.NotRange.Usage;
            const USAGE last=c.IsRange?c.Range.UsageMax:c.NotRange.Usage;
            if(c.UsagePage!=pages[target] || usages[target]<first || usages[target]>last ||
                c.BitSize==0 || c.BitSize>32 || c.ReportCount==0 || c.LogicalMax<=c.LogicalMin)continue;
            const USHORT index=static_cast<USHORT>((c.IsRange?c.Range.DataIndexMin:c.NotRange.DataIndex)+usages[target]-first);
            result.fields[target]={usages[target],c.LinkCollection,c.BitSize,index,c.LogicalMin,c.LogicalMax,c.ReportID,c.UsagePage};
            break;
        }
        if(!result.fields[target].usage)return {};
        if(target && result.fields[target].reportId!=result.fields[0].reportId)return {};
    }
    result.reportId=result.fields[0].reportId;
    // Four stick axes followed by two triggers is another standard descriptor
    // ordering. Usage Z/Rz then describe right X/Y, with Rx/Ry as triggers.
    if(!simulation && result.fields[4].index<result.fields[5].index && result.fields[5].index<result.fields[2].index) {
        const auto old=result.fields;
        result.fields[2]=old[4]; result.fields[3]=old[5];
        result.fields[4]=old[2]; result.fields[5]=old[3];
    }
    for(unsigned i=0;i<buttonCount;++i) {
        const auto& c=buttons[i];
        if(c.UsagePage==9 && c.ReportID==result.reportId && c.IsRange &&
            c.Range.UsageMin<=1 && c.Range.UsageMax>=11) {
            result.buttonLink=c.LinkCollection;
            result.androidButtons=simulation && c.Range.UsageMax>=15;
            result.valid=true;break;
        }
    }
    return result;
}
inline std::int64_t SignedValue(ULONG raw,const Field& f) {
    if(f.minimum<0 && f.bits<32 && (raw&(1u<<(f.bits-1))))
        return static_cast<std::int64_t>(raw & ((1u<<f.bits)-1))-(1LL<<f.bits);
    return f.minimum<0 ? static_cast<LONG>(raw) : static_cast<std::int64_t>(raw);
}
inline bool Decode(PHIDP_PREPARSED_DATA preparsed,const Layout& layout,
    const BYTE* data,DWORD size,XINPUT_GAMEPAD& pad) {
    if(!preparsed || !layout.valid || !data || !IsSupportedReportBuffer(size) || data[0]!=layout.reportId)return false;
    std::array<std::int64_t,7> values={};
    auto report=reinterpret_cast<PCHAR>(const_cast<BYTE*>(data));
    for(unsigned i=0;i<7;++i) {
        const auto& f=layout.fields[i];ULONG raw=0;
        if(HidP_GetUsageValue(HidP_Input,f.page,f.link,f.usage,&raw,preparsed,report,size)!=HIDP_STATUS_SUCCESS)return false;
        values[i]=SignedValue(raw,f);
        // A hat's null state lies outside its logical range and means neutral.
        if(i<6 && (values[i]<f.minimum || values[i]>f.maximum))return false;
    }
    USAGE usages[64]={};ULONG count=64;
    if(HidP_GetUsages(HidP_Input,9,layout.buttonLink,usages,&count,preparsed,report,size)!=HIDP_STATUS_SUCCESS)return false;
    XINPUT_GAMEPAD result={};
    constexpr WORD mapping[]={XINPUT_GAMEPAD_A,XINPUT_GAMEPAD_B,XINPUT_GAMEPAD_X,XINPUT_GAMEPAD_Y,
        XINPUT_GAMEPAD_LEFT_SHOULDER,XINPUT_GAMEPAD_RIGHT_SHOULDER,XINPUT_GAMEPAD_LEFT_THUMB,
        XINPUT_GAMEPAD_RIGHT_THUMB,XINPUT_GAMEPAD_BACK,0,XINPUT_GAMEPAD_START};
    // Android-compatible simulation layouts reserve C/Z and digital triggers.
    // Usage assignments follow Android CDD 7.2.6 and Generic.kl gamepad keys.
    constexpr WORD androidMapping[]={XINPUT_GAMEPAD_A,XINPUT_GAMEPAD_B,0,XINPUT_GAMEPAD_X,
        XINPUT_GAMEPAD_Y,0,XINPUT_GAMEPAD_LEFT_SHOULDER,XINPUT_GAMEPAD_RIGHT_SHOULDER,
        0,0,XINPUT_GAMEPAD_BACK,XINPUT_GAMEPAD_START,0,XINPUT_GAMEPAD_LEFT_THUMB,
        XINPUT_GAMEPAD_RIGHT_THUMB,0};
    for(ULONG i=0;i<count;++i) {
        if(layout.androidButtons) {
            if(usages[i]>=1 && usages[i]<=16)result.wButtons|=androidMapping[usages[i]-1];
        } else if(usages[i]>=1 && usages[i]<=11)result.wButtons|=mapping[usages[i]-1];
    }
    const auto hat=values[6]-layout.fields[6].minimum;
    constexpr WORD hats[]={1,9,8,10,2,6,4,5};
    if(hat>=0 && hat<8)result.wButtons|=hats[hat];
    const auto axis=[&](unsigned i,bool invert)->SHORT {
        const auto& f=layout.fields[i];
        const auto midpoint=(static_cast<std::int64_t>(f.minimum)+f.maximum+1)/2;
        const auto delta=values[i]-midpoint;
        std::int64_t scaled=delta<0?delta*32768/(midpoint-f.minimum):
            (f.maximum>midpoint?delta*32767/(f.maximum-midpoint):0);
        if(invert)scaled=-scaled;
        if(scaled>32767)scaled=32767;
        if(scaled<-32768)scaled=-32768;
        return static_cast<SHORT>(scaled);
    };
    result.sThumbLX=axis(0,false);result.sThumbLY=axis(1,true);
    result.sThumbRX=axis(2,false);result.sThumbRY=axis(3,true);
    result.bLeftTrigger=static_cast<BYTE>((values[4]-layout.fields[4].minimum)*255/
        (static_cast<std::int64_t>(layout.fields[4].maximum)-layout.fields[4].minimum));
    result.bRightTrigger=static_cast<BYTE>((values[5]-layout.fields[5].minimum)*255/
        (static_cast<std::int64_t>(layout.fields[5].maximum)-layout.fields[5].minimum));
    pad=result;return true;
}
}
