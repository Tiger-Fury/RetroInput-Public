#pragma once
#include <Windows.h>
#include <Xinput.h>
#include <cstdlib>
#include "StickWalk.h"

namespace RetroInput::Controllers {
inline bool LeftStickActive(const XINPUT_GAMEPAD& pad)
{
    return LeftStickMagnitude(pad.sThumbLX, pad.sThumbLY) > 0.15f;
}
inline bool HasActivity(const XINPUT_GAMEPAD& pad)
{
    return pad.wButtons != 0 ||
        pad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD ||
        pad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD ||
        LeftStickActive(pad) ||
        std::abs(static_cast<int>(pad.sThumbRX)) > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE ||
        std::abs(static_cast<int>(pad.sThumbRY)) > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
}
} // namespace RetroInput::Controllers
