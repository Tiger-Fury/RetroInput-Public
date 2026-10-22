#include <Windows.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <array>
#include <cassert>
#include <cstring>
static std::array<ULONG,64> rawValues;
static std::array<ULONG,256> simulationValues;
static ULONG held=0;
static bool parserFailure=false;
static NTSTATUS FakeValue(HIDP_REPORT_TYPE,USAGE page,USHORT,USAGE usage,PULONG value,PHIDP_PREPARSED_DATA,PCHAR,ULONG) {
    assert(page==1 || page==2);
    *value=page==2?simulationValues[usage]:rawValues[usage];
    return parserFailure?HIDP_STATUS_INCOMPATIBLE_REPORT_ID:HIDP_STATUS_SUCCESS;
}
static NTSTATUS FakeUsages(HIDP_REPORT_TYPE,USAGE page,USHORT,PUSAGE usages,PULONG count,PHIDP_PREPARSED_DATA,PCHAR,ULONG) {
    assert(page==9 && *count>=16);*count=0;
    for(USAGE i=1;i<=16;++i)if(held&(1u<<(i-1)))usages[(*count)++]=i;
    return HIDP_STATUS_SUCCESS;
}
#define HidP_GetUsageValue FakeValue
#define HidP_GetUsages FakeUsages
#include "../RetroInput/Core/BackboneReport.h"
#undef HidP_GetUsageValue
#undef HidP_GetUsages
#include "../RetroInput/Core/ControllerTypes.h"
#include "../RetroInput/Games/TheSuffering/InputMapping.h"
int main() {
    RetroInput::TheSuffering::InputMapping originalMapping;
    using namespace RetroInput::BackboneReports;
    assert(IsSupportedDevice(0x358A,0x0304));
    assert(!IsSupportedDevice(0x358A,0x0104) && !IsSupportedDevice(0x054C,0x0304));
    assert(RetroInput::Controllers::IsPlayStation(5));
    assert(IsSupportedReportBuffer(64) && !IsSupportedReportBuffer(7) && !IsSupportedReportBuffer(1025));
    std::array<HIDP_VALUE_CAPS,7> caps={};
    constexpr USAGE uses[]={0x30,0x31,0x32,0x33,0x34,0x35,0x39};
    for(unsigned i=0;i<7;++i) {
        auto& c=caps[i];c.UsagePage=1;c.ReportID=1;c.BitSize=8;c.ReportCount=1;
        c.LogicalMin=0;c.LogicalMax=i==6?7:255;c.NotRange.Usage=uses[i];c.NotRange.DataIndex=static_cast<USHORT>(i);
    }
    HIDP_BUTTON_CAPS buttons={};buttons.UsagePage=9;buttons.ReportID=1;buttons.IsRange=TRUE;
    buttons.Range.UsageMin=1;buttons.Range.UsageMax=12;
    auto layout=BuildLayout(caps.data(),7,&buttons,1);assert(layout.valid);
    assert(layout.fields[2].usage==0x33 && layout.fields[4].usage==0x32);
    BYTE report[64]={1};auto pp=reinterpret_cast<PHIDP_PREPARSED_DATA>(1);
    XINPUT_GAMEPAD pad={};
    constexpr WORD expected[]={0x1000,0x2000,0x4000,0x8000,0x100,0x200,0x40,0x80,0x20,0,0x10};
    constexpr WORD hats[]={1,9,8,10,2,6,4,5,0};
    for(unsigned bits=0;bits<2048;++bits)for(unsigned hat=0;hat<9;++hat) {
        held=bits;rawValues={};rawValues[0x30]=rawValues[0x31]=rawValues[0x33]=rawValues[0x34]=128;rawValues[0x39]=hat;
        assert(Decode(pp,layout,report,64,pad));
        WORD wanted=hats[hat];for(unsigned bit=0;bit<11;++bit)if(bits&(1u<<bit))wanted|=expected[bit];
        assert(pad.wButtons==wanted && !pad.sThumbLX && !pad.sThumbLY && !pad.sThumbRX && !pad.sThumbRY);
        assert(!pad.bLeftTrigger && !pad.bRightTrigger);
    }
    held=(1u<<6)|(1u<<7)|(1u<<8)|(1u<<10);rawValues[0x39]=8;
    rawValues[0x30]=0;rawValues[0x31]=255;rawValues[0x33]=255;rawValues[0x34]=0;rawValues[0x32]=255;rawValues[0x35]=128;
    assert(Decode(pp,layout,report,64,pad));
    assert(pad.sThumbLX==-32768 && pad.sThumbLY==-32767 && pad.sThumbRX==32767 && pad.sThumbRY==32767);
    assert(pad.bLeftTrigger==255 && pad.bRightTrigger==128);
    float keyboard[256]={};originalMapping.Apply(0x005B5418,keyboard,pad);
    assert(keyboard[0x2F]==1 && keyboard[0x30]==1 && keyboard[0x22]==1 && keyboard[0x01]==1);
    const auto previous=pad;parserFailure=true;assert(!Decode(pp,layout,report,64,pad));parserFailure=false;
    assert(!Decode(pp,layout,nullptr,64,pad) && !Decode(pp,layout,report,7,pad));
    report[0]=2;assert(!Decode(pp,layout,report,64,pad));report[0]=1;
    rawValues[0x30]=256;assert(!Decode(pp,layout,report,64,pad));
    assert(memcmp(&pad,&previous,sizeof(pad))==0);
    caps[0].ReportID=2;assert(!BuildLayout(caps.data(),7,&buttons,1).valid);caps[0].ReportID=1;
    assert(!BuildLayout(caps.data(),6,&buttons,1).valid);
    buttons.UsagePage=0xFF00;assert(!BuildLayout(caps.data(),7,&buttons,1).valid);buttons.UsagePage=9;
    // Alternative stick-first HID ordering keeps logical right-stick/trigger roles.
    caps[5].NotRange.DataIndex=3;caps[3].NotRange.DataIndex=4;caps[4].NotRange.DataIndex=5;
    layout=BuildLayout(caps.data(),7,&buttons,1);assert(layout.valid);
    assert(layout.fields[2].usage==0x32 && layout.fields[3].usage==0x35 && layout.fields[4].usage==0x33);
    Field signedAxis={0x30,0,16,0,-32768,32767,1};
    assert(SignedValue(0x8000,signedAxis)==-32768 && SignedValue(0xFFFF,signedAxis)==-1);
    signedAxis.bits=32;assert(SignedValue(0xFFFFFFFFu,signedAxis)==-1);

    // Reproduce the exact field pages, indices, ranges and report ID from the
    // user's SufferingBackbone.log; the previous decoder rejected this layout.
    std::array<HIDP_VALUE_CAPS,7> live={};
    constexpr USAGE liveUsages[]={0x35,0x32,0x31,0x30,0xC4,0xC5,0x39};
    for(unsigned i=0;i<7;++i) {
        auto& c=live[i];c.UsagePage=(i==4 || i==5)?2:1;c.ReportID=2;
        c.LinkCollection=1;c.ReportCount=1;c.BitSize=i<4?16:i==6?4:8;
        c.LogicalMin=i<4?-32767:0;c.LogicalMax=i<4?32767:i==6?7:255;
        c.NotRange.Usage=liveUsages[i];c.NotRange.DataIndex=static_cast<USHORT>(i);
    }
    buttons.ReportID=2;buttons.LinkCollection=1;buttons.Range.UsageMax=16;
    layout=BuildLayout(live.data(),7,&buttons,1);assert(layout.valid && layout.reportId==2);
    assert(layout.fields[2].usage==0x32 && layout.fields[3].usage==0x35);
    assert(layout.fields[4].page==2 && layout.fields[4].usage==0xC5);
    assert(layout.fields[5].page==2 && layout.fields[5].usage==0xC4);
    report[0]=2;rawValues={};simulationValues={};held=0;
    rawValues[0x39]=8;
    assert(Decode(pp,layout,report,15,pad));
    assert(!pad.wButtons && !pad.sThumbLX && !pad.sThumbLY && !pad.sThumbRX && !pad.sThumbRY);
    assert(!pad.bLeftTrigger && !pad.bRightTrigger);
    rawValues[0x30]=0x8001;rawValues[0x31]=0x7FFF;
    rawValues[0x32]=0x7FFF;rawValues[0x35]=0x8001;
    for(unsigned trigger=0;trigger<256;++trigger) {
        simulationValues[0xC5]=trigger;simulationValues[0xC4]=255-trigger;
        assert(Decode(pp,layout,report,15,pad));
        assert(pad.sThumbLX==-32768 && pad.sThumbLY==-32767 && pad.sThumbRX==32767 && pad.sThumbRY==32767);
        assert(pad.bLeftTrigger==trigger && pad.bRightTrigger==255-trigger);
    }
    rawValues={};rawValues[0x39]=8;simulationValues={};
    constexpr WORD liveExpected[]={0x1000,0x2000,0,0x4000,0x8000,0,0x100,0x200,0,0,0x20,0x10,0,0x40,0x80,0};
    assert(layout.androidButtons);
    for(unsigned bits=0;bits<65536;++bits)for(unsigned hat=0;hat<9;++hat) {
        held=bits;rawValues[0x39]=hat;
        assert(Decode(pp,layout,report,15,pad));
        WORD wanted=hats[hat];for(unsigned bit=0;bit<16;++bit)if(bits&(1u<<bit))wanted|=liveExpected[bit];
        assert(pad.wButtons==wanted);
    }
    live[5].UsagePage=1;assert(!BuildLayout(live.data(),7,&buttons,1).valid);
    live[5].UsagePage=2;live[5].ReportID=3;assert(!BuildLayout(live.data(),7,&buttons,1).valid);
}

