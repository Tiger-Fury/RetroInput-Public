#include "../RetroInput/Core/DualShock4Report.h"
#include "../RetroInput/Core/ControllerTypes.h"
#include "../RetroInput/Games/TheSuffering/InputMapping.h"
#include <array>
#include <cassert>
int main() {
    RetroInput::TheSuffering::InputMapping originalMapping;
    using namespace RetroInput::DualShock4Reports;
    assert(IsSupportedDevice(0x054C,0x05C4) && IsSupportedDevice(0x054C,0x09CC));
    assert(!IsSupportedDevice(0x054C,0x0CE6) && !IsSupportedDevice(0x1234,0x09CC));
    assert(RetroInput::Controllers::IsPlayStation(3));
    assert(RetroInput::Controllers::IsPlayStation(2));
    assert(!RetroInput::Controllers::IsPlayStation(1));
    constexpr WORD hats[9] = {1,9,8,10,2,6,4,5,0};
    assert(IsSupportedReportBuffer(547) && !IsSupportedReportBuffer(9) && !IsSupportedReportBuffer(1025));
    for (DWORD length : {10u,64u,78u,128u,547u}) {
        for (unsigned face = 0; face < 16; ++face) for (unsigned hat = 0; hat < 9; ++hat) {
            std::array<BYTE,547> data = {};
            data[0]=1; data[1]=data[2]=data[3]=data[4]=128;
            data[5]=static_cast<BYTE>((face<<4)|hat);
            data[8]=201; data[9]=101;
            XINPUT_GAMEPAD pad = {};
            assert(Decode(data.data(),length,pad));
            WORD expected=hats[hat];
            if (face&1) expected|=XINPUT_GAMEPAD_X;
            if (face&2) expected|=XINPUT_GAMEPAD_A;
            if (face&4) expected|=XINPUT_GAMEPAD_B;
            if (face&8) expected|=XINPUT_GAMEPAD_Y;
            assert(pad.wButtons==expected && pad.bLeftTrigger==201 && pad.bRightTrigger==101);
            assert(!pad.sThumbLX && !pad.sThumbLY && !pad.sThumbRX && !pad.sThumbRY);
        }
        for (unsigned bits : {1u,2u,16u,32u,64u,128u,0xF3u}) {
            BYTE data[547]={1,128,128,128,128,8}; data[6]=static_cast<BYTE>(bits);
            XINPUT_GAMEPAD pad={}; assert(Decode(data,length,pad));
            WORD expected=0;
            if(bits&1)expected|=XINPUT_GAMEPAD_LEFT_SHOULDER;
            if(bits&2)expected|=XINPUT_GAMEPAD_RIGHT_SHOULDER;
            if(bits&16)expected|=XINPUT_GAMEPAD_BACK;
            if(bits&32)expected|=XINPUT_GAMEPAD_START;
            if(bits&64)expected|=XINPUT_GAMEPAD_LEFT_THUMB;
            if(bits&128)expected|=XINPUT_GAMEPAD_RIGHT_THUMB;
            assert(pad.wButtons==expected);
        }
    }
    // CRC bytes generated independently by Python zlib over 0xA1 + first 74 bytes.
    BYTE full[78]={0x11,0x80,0,0,255,255,0,0xF7,0xF3,3,33,200};
    full[74]=0x36; full[75]=0x3f; full[76]=0xc; full[77]=0xab;
    XINPUT_GAMEPAD pad={}; assert(Decode(full,78,pad));
    assert(pad.sThumbLX==-32768 && pad.sThumbLY==-32639 && pad.sThumbRX==32639 && pad.sThumbRY==32767);
    assert(pad.bLeftTrigger==33 && pad.bRightTrigger==200);
    assert((pad.wButtons & (XINPUT_GAMEPAD_DPAD_UP|XINPUT_GAMEPAD_DPAD_LEFT))==5);
    float keyboard[256]={},mouse[8]={};
    originalMapping.Apply(0x005B5418,keyboard,pad);
    originalMapping.Apply(0x005B5440,mouse,pad);
    assert(keyboard[0x2F]==1 && keyboard[0x30]==1 && keyboard[0x22]==1 && keyboard[0x01]==1);
    assert(mouse[6]==1 && mouse[7]==1);
    // The full Bluetooth packet retains its CRC position in the padded HID buffer.
    BYTE padded[547]; memset(padded,0xCC,sizeof(padded));
    memcpy(padded,full,sizeof(full));
    assert(Decode(padded,sizeof(padded),pad));
    assert(pad.bLeftTrigger==33 && pad.bRightTrigger==200);
    padded[10]^=1; assert(!Decode(padded,sizeof(padded),pad));
    assert(!Decode(full,12,pad) && !Decode(full,77,pad));
    const WORD previous=pad.wButtons;
    full[10]^=1;
    assert(!Decode(full,78,pad) && pad.wButtons==previous);
    assert(!Decode(full,77,pad) && !Decode(nullptr,0,pad));
    full[0]=0x31; assert(!Decode(full,78,pad));
    return 0;
}

