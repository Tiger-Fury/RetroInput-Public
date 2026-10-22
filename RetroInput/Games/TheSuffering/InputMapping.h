#pragma once
#include <Windows.h>
#include <Xinput.h>
#include "../../Core/StickWalk.h"

// The Suffering (2004) game-specific bindings. Device records and state indices
// are specific to this game's input hook, not the reusable controller core.
namespace RetroInput::TheSuffering {
class InputMapping {
public:
    void Reset() { walk_.Reset(); wasWalking_ = stopping_ = false; stoppedAt_ = 0; }
    void Apply(DWORD deviceRecord, float* state, const XINPUT_GAMEPAD& pad, DWORD now = GetTickCount())
    {
        if (!state) return;
        // Helper: merge a button press with existing keyboard input.
        auto Key = [&](int scanCode, bool pressed)
            {
                if (pressed)
                    state[scanCode] = 1.0f;
            };

        // KEYBOARD DEVICE
        if (deviceRecord == 0x005B5418)
        {
            const bool walking = walk_.Update(pad.sThumbLX, pad.sThumbLY);
            if (walk_.Moving()) {
                stopping_ = false; // Moving into the run range releases Shift immediately.
                // Eight movement directions, with a circular deadzone. The sector
                // test prevents tiny perpendicular noise adding a diagonal key.
                const float x = pad.sThumbLX / (pad.sThumbLX < 0 ? 32768.0f : 32767.0f);
                const float y = pad.sThumbLY / (pad.sThumbLY < 0 ? 32768.0f : 32767.0f);
                const float sector = std::sqrt(x * x + y * y) * 0.38268343f;
                Key(0xC8, y > sector);
                Key(0xD0, y < -sector);
                Key(0xCB, x < -sector);
                Key(0xCD, x > sector);
            }
            else if (wasWalking_) {
                stoppedAt_ = now;
                stopping_ = true;
            }
            if (stopping_ && static_cast<DWORD>(now - stoppedAt_) >= StopHoldMs)
                stopping_ = false;
            // Let the walk-to-idle transition finish before dropping its modifier.
            // Deadzone noise cannot start or extend this hold.
            Key(0x2A, walking || stopping_);
            wasWalking_ = walking;

            // Normalized face buttons
            Key(0x39, pad.wButtons & XINPUT_GAMEPAD_A); // Space: Jump
            Key(0x13, pad.wButtons & XINPUT_GAMEPAD_B); // R: Reload
            Key(0x1C, pad.wButtons & XINPUT_GAMEPAD_X); // Enter: Interact
            Key(0x2E, pad.wButtons & XINPUT_GAMEPAD_Y); // C: Insanity

            // Shoulder buttons
            Key(0x0F, pad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER); // Tab
            Key(0x1D, pad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);  // Left Ctrl

            // Stick clicks use the shared normalized input state.
            Key(0x2F, pad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB);  // V: L3 / left-stick click
            Key(0x30, pad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB); // B: R3 / right-stick click

            // Menu buttons
            Key(0x01, pad.wButtons & XINPUT_GAMEPAD_START); // Escape
            Key(0x22, pad.wButtons & XINPUT_GAMEPAD_BACK);  // G

            // D-pad
            Key(0x17, pad.wButtons & XINPUT_GAMEPAD_DPAD_UP);    // I
            Key(0x1A, pad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT);  // [
            Key(0x1B, pad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT); // ]
            Key(0x21, pad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN);  // F
        }

        // MOUSE DEVICE
        else if (deviceRecord == 0x005B5440)
        {
            const float deadzone =
                static_cast<float>(
                    XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);

            const float sensitivity = 0.75f;

            float fx = static_cast<float>(pad.sThumbRX);
            float fy = static_cast<float>(pad.sThumbRY);

            fx = (fx > deadzone)
                ? (fx - deadzone) / (32767.0f - deadzone)
                : (fx < -deadzone)
                ? (fx + deadzone) / (32768.0f - deadzone)
                : 0.0f;

            fy = (fy > deadzone)
                ? (fy - deadzone) / (32767.0f - deadzone)
                : (fy < -deadzone)
                ? (fy + deadzone) / (32768.0f - deadzone)
                : 0.0f;

            fx *= sensitivity;
            fy *= sensitivity;

            // Right analog stick: camera
            if (fx < 0.0f)
                state[2] += -fx;
            else
                state[3] += fx;

            if (fy > 0.0f)
                state[0] += fy;
            else
                state[1] += -fy;

            // Triggers: mouse buttons
            const BYTE triggerThreshold =
                XINPUT_GAMEPAD_TRIGGER_THRESHOLD;

            // RT: Fire (left mouse button)
            if (pad.bRightTrigger > triggerThreshold)
                state[6] = 1.0f;

            // LT: Alt Fire (right mouse button)
            if (pad.bLeftTrigger > triggerThreshold)
                state[7] = 1.0f;
        }
    }
private:
    static constexpr DWORD StopHoldMs = 250;
    Controllers::StickWalk walk_;
    bool wasWalking_ = false, stopping_ = false;
    DWORD stoppedAt_ = 0;
};
} // namespace RetroInput::TheSuffering
