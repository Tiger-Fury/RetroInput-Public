#pragma once
#include <Windows.h>
#include <Xinput.h>

struct DualSenseSnapshot {
    bool connected = false;
    DWORD lastActivity = 0;
    XINPUT_GAMEPAD gamepad = {};
};

bool StartDualSenseReader();
DualSenseSnapshot GetDualSenseSnapshot();
void ResetDualSenseActivity();
