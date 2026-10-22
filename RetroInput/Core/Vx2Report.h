#pragma once
#include <Windows.h>
#include <Xinput.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <array>
#include <cstdint>

namespace RetroInput::Vx2Reports {
inline bool IsSupportedDevice(USHORT vendor,USHORT product) {return vendor==0x25F0 && product==0x83C3;}
inline bool IsSupportedReportBuffer(DWORD size) {return size>=6 && size<=1024;}
struct Field {
    USAGE usage=0;
    USHORT link=0,bits=0;
    LONG minimum=0,maximum=0;
    BYTE reportId=0;
};
struct Layout {
    std::array<Field,5> fields={}; // LX,LY,RX,RY,hat.
    USHORT buttonLink=0;
    BYTE reportId=0;
    bool valid=false;
};
inline Layout BuildLayout(const HIDP_VALUE_CAPS* values,unsigned count,
    const HIDP_BUTTON_CAPS* buttons,unsigned buttonCount) {
    const auto find=[&](USAGE usage)->Field {
        for(unsigned i=0;i<count;++i) {
            const auto& c=values[i];
            const USAGE first=c.IsRange?c.Range.UsageMin:c.NotRange.Usage;
            const USAGE last=c.IsRange?c.Range.UsageMax:c.NotRange.Usage;
            if(c.UsagePage==1 && usage>=first && usage<=last && c.BitSize>0 &&
                c.BitSize<=32 && c.ReportCount && c.LogicalMax>c.LogicalMin)
                return {usage,c.LinkCollection,c.BitSize,c.LogicalMin,c.LogicalMax,c.ReportID};
        }
        return {};
    };
    Layout result;
    result.fields={find(0x30),find(0x31),find(0x32),find(0x35),find(0x39)};
    if(!result.fields[2].usage || !result.fields[3].usage) {
        result.fields[2]=find(0x33);result.fields[3]=find(0x34);
    }
    result.reportId=result.fields[0].reportId;
    for(const auto& f:result.fields)if(!f.usage || f.reportId!=result.reportId)return {};
    for(unsigned i=0;i<buttonCount;++i) {
        const auto& c=buttons[i];
        if(c.UsagePage==9 && c.ReportID==result.reportId && c.IsRange &&
            c.Range.UsageMin<=1 && c.Range.UsageMax>=12) {
            result.buttonLink=c.LinkCollection;result.valid=true;break;
        }
    }
    return result;
}
inline bool Decode(PHIDP_PREPARSED_DATA preparsed,const Layout& layout,
    const BYTE* data,DWORD size,XINPUT_GAMEPAD& pad) {
    if(!preparsed || !layout.valid || !data || !IsSupportedReportBuffer(size) || data[0]!=layout.reportId)return false;
    auto report=reinterpret_cast<PCHAR>(const_cast<BYTE*>(data));
    std::array<std::int64_t,5> values={};
    for(unsigned i=0;i<5;++i) {
        const auto& f=layout.fields[i];ULONG raw=0;
        if(HidP_GetUsageValue(HidP_Input,1,f.link,f.usage,&raw,preparsed,report,size)!=HIDP_STATUS_SUCCESS)return false;
        std::int64_t value=raw;
        if(f.minimum<0) {
            if(f.bits<32 && (raw&(1u<<(f.bits-1))))value=static_cast<std::int64_t>(raw&((1u<<f.bits)-1))-(1LL<<f.bits);
            else value=static_cast<LONG>(raw);
        }
        if(i<4 && (value<f.minimum || value>f.maximum))return false;
        values[i]=value;
    }
    USAGE usages[64]={};ULONG count=64;
    if(HidP_GetUsages(HidP_Input,9,layout.buttonLink,usages,&count,preparsed,report,size)!=HIDP_STATUS_SUCCESS)return false;
    XINPUT_GAMEPAD result={};
    // SDL's 25F0:83C3 mapping: Triangle,Circle,Cross,Square,L1,R1,L2,R2,
    // Select,Start,L3,R3. Triggers are digital buttons, not invented analogue axes.
    constexpr WORD mapping[]={XINPUT_GAMEPAD_Y,XINPUT_GAMEPAD_B,XINPUT_GAMEPAD_A,XINPUT_GAMEPAD_X,
        XINPUT_GAMEPAD_LEFT_SHOULDER,XINPUT_GAMEPAD_RIGHT_SHOULDER,0,0,
        XINPUT_GAMEPAD_BACK,XINPUT_GAMEPAD_START,XINPUT_GAMEPAD_LEFT_THUMB,XINPUT_GAMEPAD_RIGHT_THUMB};
    for(ULONG i=0;i<count;++i) {
        if(usages[i]>=1 && usages[i]<=12)result.wButtons|=mapping[usages[i]-1];
        if(usages[i]==7)result.bLeftTrigger=255;
        if(usages[i]==8)result.bRightTrigger=255;
    }
    constexpr WORD hats[]={1,9,8,10,2,6,4,5};
    const auto hat=values[4]-layout.fields[4].minimum;
    if(hat>=0 && hat<8)result.wButtons|=hats[hat];
    const auto axis=[&](unsigned i,bool invert)->SHORT {
        const auto& f=layout.fields[i];
        const auto middle=(static_cast<std::int64_t>(f.minimum)+f.maximum+1)/2;
        const auto delta=values[i]-middle;
        auto value=delta<0?delta*32768/(middle-f.minimum):
            (f.maximum>middle?delta*32767/(f.maximum-middle):0);
        if(invert)value=-value;
        if(value>32767)value=32767;if(value<-32768)value=-32768;
        return static_cast<SHORT>(value);
    };
    result.sThumbLX=axis(0,false);result.sThumbLY=axis(1,true);
    result.sThumbRX=axis(2,false);result.sThumbRY=axis(3,true);
    pad=result;return true;
}
}
