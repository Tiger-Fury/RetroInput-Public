#pragma once
#include <Windows.h>
#include <Xinput.h>
#include <array>
#include <cstdint>
#include <cstdlib>
#include "ControllerActivity.h"

namespace RetroController {
struct XInputSnapshot {
    bool connected = false;
    XINPUT_GAMEPAD gamepad = {};
};

// All XInput slots share one player's input. Analogue ownership applies to both
// sticks together; button/trigger input never changes that ownership.
class XInputAggregator {
public:
    XInputSnapshot Update(const std::array<XINPUT_STATE, XUSER_MAX_COUNT>& states,
        const std::array<bool, XUSER_MAX_COUNT>& connected) {
        XInputSnapshot result;
        for (unsigned i = 0; i < XUSER_MAX_COUNT; ++i) {
            auto& slot = slots_[i];
            if (!connected[i]) { slot = {}; continue; }
            result.connected = true;
            const auto& pad = states[i].Gamepad;
            result.gamepad.wButtons |= pad.wButtons;
            if (pad.bLeftTrigger > result.gamepad.bLeftTrigger)
                result.gamepad.bLeftTrigger = pad.bLeftTrigger;
            if (pad.bRightTrigger > result.gamepad.bRightTrigger)
                result.gamepad.bRightTrigger = pad.bRightTrigger;
            const bool active = SticksActive(pad);
            // Accumulate small changes against the last meaningful position.
            // A held stick's noise must not keep stealing ownership from another pad.
            const bool moved = active && (!slot.active || Changed(pad, slot.anchor));
            if (moved) slot.order = ++sequence_;
            if (moved || !active) slot.anchor = pad;
            slot.active = active;
        }
        if (owner_ >= XUSER_MAX_COUNT || !slots_[owner_].active) owner_ = XUSER_MAX_COUNT;
        for (unsigned i = 0; i < XUSER_MAX_COUNT; ++i)
            if (slots_[i].active && (owner_ >= XUSER_MAX_COUNT || slots_[i].order > slots_[owner_].order))
                owner_ = i;
        if (owner_ < XUSER_MAX_COUNT) {
            const auto& pad = states[owner_].Gamepad;
            result.gamepad.sThumbLX = pad.sThumbLX;
            result.gamepad.sThumbLY = pad.sThumbLY;
            result.gamepad.sThumbRX = pad.sThumbRX;
            result.gamepad.sThumbRY = pad.sThumbRY;
        }
        return result;
    }
    void Reset() { slots_ = {}; owner_ = XUSER_MAX_COUNT; sequence_ = 0; }
private:
    static bool SticksActive(const XINPUT_GAMEPAD& p) {
        return RetroInput::Controllers::LeftStickActive(p) ||
            std::abs(static_cast<int>(p.sThumbRX)) > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE ||
            std::abs(static_cast<int>(p.sThumbRY)) > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
    }
    static bool Changed(const XINPUT_GAMEPAD& a, const XINPUT_GAMEPAD& b) {
        constexpr int movementThreshold = 2048;
        const auto axis = [](SHORT value, int deadzone) {
            return std::abs(static_cast<int>(value)) > deadzone ? static_cast<int>(value) : 0;
        };
        return std::abs(static_cast<int>(a.sThumbLX) - b.sThumbLX) >= movementThreshold ||
            std::abs(static_cast<int>(a.sThumbLY) - b.sThumbLY) >= movementThreshold ||
            std::abs(axis(a.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) -
                     axis(b.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)) >= movementThreshold ||
            std::abs(axis(a.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) -
                     axis(b.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)) >= movementThreshold;
    }
    struct Slot { bool active = false; std::uint64_t order = 0; XINPUT_GAMEPAD anchor = {}; };
    std::array<Slot, XUSER_MAX_COUNT> slots_ = {};
    unsigned owner_ = XUSER_MAX_COUNT;
    std::uint64_t sequence_ = 0;
};
} // namespace RetroController
