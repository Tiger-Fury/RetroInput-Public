#pragma once
#include <Windows.h>
#include <Xinput.h>
#include "../../Core/StickWalk.h"
#include "../../Core/TapHold.h"

namespace RetroInput::TheSufferingTTB {
inline constexpr DWORD KeyboardRecord = 0x006156EC;
inline constexpr DWORD MouseRecord = 0x00615714;

class InputMapping {
public:
    void Reset()
    {
        walk_.Reset();
        pickup_.Reset();
        wasWalking_ = stopping_ = false;
        stoppedAt_ = 0;
    }
    void Apply(DWORD record, float* state, const XINPUT_GAMEPAD& pad, DWORD now = GetTickCount())
    {
        if (!state) return;
        auto key = [&](unsigned scan, bool pressed) { if (pressed) state[scan] = 1.0f; };
        if (record == KeyboardRecord) {
            const bool walking = walk_.Update(pad.sThumbLX, pad.sThumbLY);
            if (walk_.Moving()) {
                stopping_ = false; // Moving into the run range releases Shift immediately.
                // Eight movement directions, with a circular deadzone. The sector
                // test prevents tiny perpendicular noise adding a diagonal key.
                const float x = pad.sThumbLX / (pad.sThumbLX < 0 ? 32768.0f : 32767.0f);
                const float y = pad.sThumbLY / (pad.sThumbLY < 0 ? 32768.0f : 32767.0f);
                const float sector = std::sqrt(x * x + y * y) * 0.38268343f;
                key(0xC8, y > sector);
                key(0xD0, y < -sector);
                key(0xCB, x < -sector);
                key(0xCD, x > sector);
            }
            else if (wasWalking_) {
                stoppedAt_ = now;
                stopping_ = true;
            }
            if (stopping_ && static_cast<DWORD>(now - stoppedAt_) >= StopHoldMs)
                stopping_ = false;
            // Let the walk-to-idle transition finish before dropping its modifier.
            // Deadzone noise cannot start or extend this hold.
            key(0x2A, walking || stopping_);
            wasWalking_ = walking;
            key(0x1D, pad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB);  // Crouch
            key(0x21, pad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB); // Flashlight
            key(0x1C, pad.wButtons & XINPUT_GAMEPAD_X);           // Interact
            key(0x39, pad.wButtons & XINPUT_GAMEPAD_A);           // Jump
            const auto pickup = pickup_.Update((pad.wButtons & XINPUT_GAMEPAD_Y) != 0, now);
            key(0x10, pickup.tap);  // Tap release: cycle weapons (Q).
            key(0x2C, pickup.hold); // Hold: pick up (Z), without cycling first.
            key(0x2E, pad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);  // Insanity
            key(0x2D, pad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER); // Cycle thrown weapons
            key(0x01, pad.wButtons & XINPUT_GAMEPAD_START);       // Pause
            key(0x03, pad.wButtons & XINPUT_GAMEPAD_BACK);        // Map (2)
            key(0x02, pad.wButtons & XINPUT_GAMEPAD_DPAD_UP);     // First person (1)
            key(0x13, pad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN);   // Reload
            key(0x4B, pad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT);   // Numpad 4
            key(0x4D, pad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);  // Numpad 6
        }
        else if (record == MouseRecord) {
            const auto axis = [](SHORT value) {
                constexpr float dz = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
                return value > dz ? (value - dz) / (32767.0f - dz) :
                    value < -dz ? (value + dz) / (32768.0f - dz) : 0.0f;
            };
            const float x = axis(pad.sThumbRX) * 0.75f;
            const float y = axis(pad.sThumbRY) * 0.75f;
            state[x < 0 ? 2 : 3] += std::abs(x);
            state[y > 0 ? 0 : 1] += std::abs(y);
            key(6, pad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD); // Attack
            key(7, pad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD);  // Throw
            key(8, pad.wButtons & XINPUT_GAMEPAD_B);                     // Secondary attack
        }
    }
private:
    static constexpr DWORD StopHoldMs = 250;
    Controllers::StickWalk walk_;
    Controllers::TapHold pickup_;
    bool wasWalking_ = false;
    bool stopping_ = false;
    DWORD stoppedAt_ = 0;
};
} // namespace RetroInput::TheSufferingTTB

