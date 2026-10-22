#pragma once
#include <Windows.h>
#include <Xinput.h>
struct BackboneSnapshot {
    bool connected = false;
    DWORD lastActivity = 0;
    XINPUT_GAMEPAD gamepad = {};
    USHORT productId = 0;
    bool bluetooth = false;
};
bool StartBackboneReader();
BackboneSnapshot GetBackboneSnapshot();
void ResetBackboneActivity();
