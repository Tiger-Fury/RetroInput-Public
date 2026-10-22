#include "../RetroInput/Core/ShieldReport.h"
#include "../RetroInput/Core/ControllerTypes.h"
#include "../RetroInput/Games/TheSuffering/InputMapping.h"
#include <array>
#include <cassert>
#include <cstring>
int main() {
    RetroInput::TheSuffering::InputMapping originalMapping;
    using namespace RetroInput::ShieldReports;
    assert(IsSupportedDevice(0x0955,0x7214));
    assert(!IsSupportedDevice(0x0955,0x7210) && !IsSupportedDevice(0x054C,0x7214));
    assert(!RetroInput::Controllers::IsPlayStation(4));
    assert(IsSupportedReportBuffer(33) && !IsSupportedReportBuffer(22) && !IsSupportedReportBuffer(1025));
    constexpr WORD hats[] = {1,9,8,10,2,6,4,5,0};
    constexpr WORD buttons[] = {0x1000,0x2000,0x4000,0x8000,0x100,0x200,0x40,0x80};
    for (DWORD size : {23u,33u,64u,128u}) {
        for (unsigned hat = 0; hat < 9; ++hat) for (unsigned bits = 0; bits < 256; ++bits) {
            std::array<BYTE,128> report = {};
            report[0]=1; report[2]=static_cast<BYTE>(hat); report[3]=static_cast<BYTE>(bits);
            report[10]=report[12]=report[14]=report[16]=128;
            XINPUT_GAMEPAD pad = {}; assert(Decode(report.data(),size,pad));
            WORD expected=hats[hat];
            for(unsigned bit=0;bit<8;++bit) if(bits&(1u<<bit))expected|=buttons[bit];
            assert(pad.wButtons == expected);
            assert(!pad.sThumbLX && !pad.sThumbLY && !pad.sThumbRX && !pad.sThumbRY);
            assert(!pad.bLeftTrigger && !pad.bRightTrigger);
        }
    }
    BYTE report[33] = {1,0,8,0xC0,1};
    report[9]=0;report[10]=0;report[11]=0xFF;report[12]=0xFF;
    report[13]=0xFF;report[14]=0xFF;report[15]=0;report[16]=0;
    report[17]=2;report[19]=report[20]=0xFF;report[21]=0;report[22]=0x80;
    XINPUT_GAMEPAD pad={}; assert(Decode(report,33,pad));
    assert(pad.sThumbLX==-32768 && pad.sThumbLY==-32767 && pad.sThumbRX==32767 && pad.sThumbRY==32767);
    assert(pad.bLeftTrigger==255 && pad.bRightTrigger==127);
    assert(pad.wButtons==(XINPUT_GAMEPAD_START|XINPUT_GAMEPAD_BACK|XINPUT_GAMEPAD_LEFT_THUMB|XINPUT_GAMEPAD_RIGHT_THUMB));
    float keyboard[256]={};
    originalMapping.Apply(0x005B5418,keyboard,pad);
    assert(keyboard[0x2F]==1 && keyboard[0x30]==1 && keyboard[0x01]==1 && keyboard[0x22]==1);
    report[3]=report[4]=0;report[17]=5; // Voice/Home are not gameplay buttons.
    assert(Decode(report,33,pad) && pad.wButtons==0);
    const auto previous=pad;
    assert(!Decode(nullptr,33,pad) && !Decode(report,22,pad));
    for (BYTE id : {BYTE(0),BYTE(2),BYTE(3),BYTE(4)}) {report[0]=id;assert(!Decode(report,33,pad));}
    assert(memcmp(&pad,&previous,sizeof(pad))==0);
}

