#pragma once
#include <Windows.h>
#include <Xinput.h>
struct DualShock4Snapshot {
    bool connected = false;
    DWORD lastActivity = 0;
    XINPUT_GAMEPAD gamepad = {};
    USHORT productId = 0;
    bool bluetooth = false;
};
bool StartDualShock4Reader();
DualShock4Snapshot GetDualShock4Snapshot();
void ResetDualShock4Activity();
