#pragma once
#include <Windows.h>
#include <Xinput.h>
#include <cstddef>
#include <cstdint>

// Original Switch Joy-Con Bluetooth full reports. Protocol reference:
// https://github.com/libsdl-org/SDL/blob/main/src/joystick/hidapi/SDL_hidapi_switch.c
namespace RetroInput::JoyConReports {
inline bool IsSupportedDevice(USHORT vendor, USHORT product) {
    return vendor == 0x057E && (product == 0x2006 || product == 0x2007);
}
inline unsigned PackedX(const BYTE* p) { return p[0] | ((p[1] & 15u) << 8); }
inline unsigned PackedY(const BYTE* p) { return (p[1] >> 4) | (unsigned(p[2]) << 4); }
struct Axis {
    int center = 2048, below = 1400, above = 1400;
    bool Valid() const {
        return center > 0 && center < 4095 && below > 0 && above > 0 &&
            below <= center && above <= 4095 - center;
    }
    SHORT Normalize(unsigned raw) const {
        const int delta = int(raw) - center;
        const int range = delta < 0 ? below : above;
        if (delta <= -range) return -32768;
        if (delta >= range) return 32767;
        return static_cast<SHORT>(delta * (delta < 0 ? 32768 : 32767) / range);
    }
};
struct Calibration { Axis x, y; };
inline bool ReadCalibration(const BYTE* data, std::size_t length, bool right, Calibration& out) {
    if (!data || length != 9) return false;
    const int center = right ? 0 : 3, below = right ? 3 : 6, above = right ? 6 : 0;
    Calibration candidate;
    candidate.x = {int(PackedX(data+center)), int(PackedX(data+below)), int(PackedX(data+above))};
    candidate.y = {int(PackedY(data+center)), int(PackedY(data+below)), int(PackedY(data+above))};
    if (!candidate.x.Valid() || !candidate.y.Valid()) return false;
    out = candidate;
    return true;
}
inline bool Decode(const BYTE* report, std::size_t length, bool right,
    const Calibration& calibration, XINPUT_GAMEPAD& out) {
    out = {};
    if (!report || length < 13 || report[0] != 0x30 ||
        !calibration.x.Valid() || !calibration.y.Valid()) return false;
    const BYTE common = report[4], buttons = report[right ? 3 : 5];
    if (right) {
        // Positional mapping: Nintendo B/A/Y/X -> normalized A/B/X/Y.
        if (buttons & 4) out.wButtons |= XINPUT_GAMEPAD_A;
        if (buttons & 8) out.wButtons |= XINPUT_GAMEPAD_B;
        if (buttons & 1) out.wButtons |= XINPUT_GAMEPAD_X;
        if (buttons & 2) out.wButtons |= XINPUT_GAMEPAD_Y;
        if (buttons & 0x40) out.wButtons |= XINPUT_GAMEPAD_RIGHT_SHOULDER;
        if (buttons & 0x80) out.bRightTrigger = 255;
        if (common & 2) out.wButtons |= XINPUT_GAMEPAD_START;
        if (common & 4) out.wButtons |= XINPUT_GAMEPAD_RIGHT_THUMB;
        out.sThumbRX = calibration.x.Normalize(PackedX(report+9));
        out.sThumbRY = calibration.y.Normalize(PackedY(report+9));
    } else {
        if (buttons & 1) out.wButtons |= XINPUT_GAMEPAD_DPAD_DOWN;
        if (buttons & 2) out.wButtons |= XINPUT_GAMEPAD_DPAD_UP;
        if (buttons & 4) out.wButtons |= XINPUT_GAMEPAD_DPAD_RIGHT;
        if (buttons & 8) out.wButtons |= XINPUT_GAMEPAD_DPAD_LEFT;
        if (buttons & 0x40) out.wButtons |= XINPUT_GAMEPAD_LEFT_SHOULDER;
        if (buttons & 0x80) out.bLeftTrigger = 255;
        if (common & 1) out.wButtons |= XINPUT_GAMEPAD_BACK;
        if (common & 8) out.wButtons |= XINPUT_GAMEPAD_LEFT_THUMB;
        out.sThumbLX = calibration.x.Normalize(PackedX(report+6));
        out.sThumbLY = calibration.y.Normalize(PackedY(report+6));
    }
    return true;
}
struct Half {
    bool connected = false;
    DWORD received = 0, activity = 0;
    XINPUT_GAMEPAD pad = {};
};
inline bool Combine(const Half& left, const Half& right, DWORD now,
    XINPUT_GAMEPAD& out, DWORD& activity) {
    out = {}; activity = 0;
    // Full-controller mode requires both halves. Expiry also clears held input
    // if Bluetooth disappears before Windows reports a disconnect.
    if (!left.connected || !right.connected || DWORD(now-left.received) >= 1000 ||
        DWORD(now-right.received) >= 1000) return false;
    out.wButtons = left.pad.wButtons | right.pad.wButtons;
    out.bLeftTrigger = left.pad.bLeftTrigger;
    out.bRightTrigger = right.pad.bRightTrigger;
    out.sThumbLX = left.pad.sThumbLX; out.sThumbLY = left.pad.sThumbLY;
    out.sThumbRX = right.pad.sThumbRX; out.sThumbRY = right.pad.sThumbRY;
    activity = !left.activity ? right.activity : !right.activity ? left.activity :
        LONG(left.activity-right.activity) > 0 ? left.activity : right.activity;
    return true;
}
} // namespace RetroInput::JoyConReports
