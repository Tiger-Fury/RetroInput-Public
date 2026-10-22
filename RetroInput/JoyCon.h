#pragma once
#include <Windows.h>
#include <Xinput.h>
struct JoyConSnapshot {
    bool connected = false;
    DWORD lastActivity = 0;
    XINPUT_GAMEPAD gamepad = {};
};
bool StartJoyConReader();
JoyConSnapshot GetJoyConSnapshot();
void ResetJoyConActivity();
