#pragma once
#include <Windows.h>
#include <Xinput.h>
struct ShieldSnapshot {
    bool connected = false;
    DWORD lastActivity = 0;
    XINPUT_GAMEPAD gamepad = {};
    USHORT productId = 0;
    bool bluetooth = false;
};
bool StartShieldReader();
ShieldSnapshot GetShieldSnapshot();
void ResetShieldActivity();
