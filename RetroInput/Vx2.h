#pragma once
#include <Windows.h>
#include <Xinput.h>
struct Vx2Snapshot {
    bool connected = false;
    DWORD lastActivity = 0;
    XINPUT_GAMEPAD gamepad = {};
    USHORT productId = 0;
    bool bluetooth = false;
};
bool StartVx2Reader();
Vx2Snapshot GetVx2Snapshot();
void ResetVx2Activity();
