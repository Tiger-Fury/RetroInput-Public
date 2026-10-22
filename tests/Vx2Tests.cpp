#include <Windows.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <array>
#include <cassert>
#include <cstring>
static std::array<ULONG,64> rawValues;
static unsigned held=0;
static bool parseFails=false;
static NTSTATUS FakeValue(HIDP_REPORT_TYPE,USAGE page,USHORT,USAGE usage,PULONG value,PHIDP_PREPARSED_DATA,PCHAR,ULONG) {
    assert(page==1);*value=rawValues[usage];return parseFails?HIDP_STATUS_INCOMPATIBLE_REPORT_ID:HIDP_STATUS_SUCCESS;
}
static NTSTATUS FakeUsages(HIDP_REPORT_TYPE,USAGE page,USHORT,PUSAGE usages,PULONG count,PHIDP_PREPARSED_DATA,PCHAR,ULONG) {
    assert(page==9 && *count>=13);*count=0;
    for(USAGE i=1;i<=13;++i)if(held&(1u<<(i-1)))usages[(*count)++]=i;
    return HIDP_STATUS_SUCCESS;
}
#define HidP_GetUsageValue FakeValue
#define HidP_GetUsages FakeUsages
#include "../RetroInput/Core/Vx2Report.h"
#undef HidP_GetUsageValue
#undef HidP_GetUsages
#include "../RetroInput/Core/ControllerTypes.h"
#include "../RetroInput/Games/TheSuffering/InputMapping.h"
int main() {
    RetroInput::TheSuffering::InputMapping originalMapping;
    using namespace RetroInput::Vx2Reports;
    assert(IsSupportedDevice(0x25F0,0x83C3));
    assert(!IsSupportedDevice(0x25F0,0x83C4) && !IsSupportedDevice(0x054C,0x83C3));
    assert(RetroInput::Controllers::IsPlayStation(6));
    assert(IsSupportedReportBuffer(19) && !IsSupportedReportBuffer(5) && !IsSupportedReportBuffer(1025));
    constexpr USAGE fields[]={0x30,0x31,0x32,0x35,0x39};
    std::array<HIDP_VALUE_CAPS,5> caps={};
    for(unsigned i=0;i<5;++i) {
        auto& c=caps[i];c.UsagePage=1;c.ReportID=0;c.BitSize=i==4?4:8;c.ReportCount=1;
        c.LogicalMin=0;c.LogicalMax=i==4?7:255;c.NotRange.Usage=fields[i];
    }
    HIDP_BUTTON_CAPS buttons={};buttons.UsagePage=9;buttons.IsRange=TRUE;
    buttons.Range.UsageMin=1;buttons.Range.UsageMax=13;
    auto layout=BuildLayout(caps.data(),5,&buttons,1);assert(layout.valid);
    BYTE report[64]={};auto pp=reinterpret_cast<PHIDP_PREPARSED_DATA>(1);
    constexpr WORD expected[]={0x8000,0x2000,0x1000,0x4000,0x100,0x200,0,0,0x20,0x10,0x40,0x80,0};
    constexpr WORD hats[]={1,9,8,10,2,6,4,5,0};
    XINPUT_GAMEPAD pad={};
    for(DWORD size:{8u,19u,64u})for(unsigned bits=0;bits<8192;++bits)for(unsigned hat=0;hat<9;++hat) {
        held=bits;rawValues={};rawValues[0x30]=rawValues[0x31]=rawValues[0x32]=rawValues[0x35]=128;
        rawValues[0x39]=hat;assert(Decode(pp,layout,report,size,pad));
        WORD wanted=hats[hat];for(unsigned bit=0;bit<13;++bit)if(bits&(1u<<bit))wanted|=expected[bit];
        assert(pad.wButtons==wanted && pad.bLeftTrigger==(bits&64?255:0) && pad.bRightTrigger==(bits&128?255:0));
        assert(!pad.sThumbLX && !pad.sThumbLY && !pad.sThumbRX && !pad.sThumbRY);
    }
    held=(1u<<8)|(1u<<9)|(1u<<10)|(1u<<11);rawValues[0x39]=8;
    rawValues[0x30]=0;rawValues[0x31]=255;rawValues[0x32]=255;rawValues[0x35]=0;
    assert(Decode(pp,layout,report,19,pad));
    assert(pad.sThumbLX==-32768 && pad.sThumbLY==-32767 && pad.sThumbRX==32767 && pad.sThumbRY==32767);
    float keyboard[256]={};originalMapping.Apply(0x005B5418,keyboard,pad);
    assert(keyboard[0x2F]==1 && keyboard[0x30]==1 && keyboard[0x01]==1 && keyboard[0x22]==1);
    const auto previous=pad;parseFails=true;assert(!Decode(pp,layout,report,19,pad));parseFails=false;
    report[0]=1;assert(!Decode(pp,layout,report,19,pad));report[0]=0;
    assert(!Decode(pp,layout,nullptr,19,pad) && !Decode(pp,layout,report,5,pad));
    rawValues[0x30]=256;assert(!Decode(pp,layout,report,19,pad));
    assert(memcmp(&pad,&previous,sizeof(pad))==0);
    assert(!BuildLayout(caps.data(),4,&buttons,1).valid);
    caps[0].ReportID=1;assert(!BuildLayout(caps.data(),5,&buttons,1).valid);caps[0].ReportID=0;
    buttons.Range.UsageMax=10;assert(!BuildLayout(caps.data(),5,&buttons,1).valid);buttons.Range.UsageMax=13;
    caps[2].NotRange.Usage=0x33;caps[3].NotRange.Usage=0x34;
    layout=BuildLayout(caps.data(),5,&buttons,1);assert(layout.valid && layout.fields[2].usage==0x33);
    for(auto& c:caps){if(c.NotRange.Usage!=0x39){c.BitSize=16;c.LogicalMin=-32767;c.LogicalMax=32767;}}
    layout=BuildLayout(caps.data(),5,&buttons,1);assert(layout.valid);
    rawValues={};rawValues[0x39]=8;held=0;
    assert(Decode(pp,layout,report,19,pad) && pad.sThumbLX==0);
    rawValues[0x30]=0x8001;assert(Decode(pp,layout,report,19,pad) && pad.sThumbLX==-32768);
}

