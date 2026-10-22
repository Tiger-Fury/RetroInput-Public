#pragma once
#include <Windows.h>
#include <Xinput.h>
#include <cstdint>

// Original normalized decoder for NVIDIA Controller v01.04 (2017).
// Report offsets cross-checked against SDL's SDL_hidapi_shield.c V104 reader.
// USB and Bluetooth use the same state layout; no feature/output writes.
namespace RetroInput::ShieldReports {
inline bool IsSupportedDevice(USHORT vendor, USHORT product) {
    return vendor == 0x0955 && product == 0x7214;
}
inline bool IsSupportedReportBuffer(DWORD size) { return size >= 23 && size <= 1024; }
inline bool Decode(const BYTE* data, DWORD size, XINPUT_GAMEPAD& pad) {
    if (!data || !IsSupportedReportBuffer(size) || data[0] != 1) return false;
    const auto word = [data](unsigned offset) {
        return static_cast<unsigned>(data[offset]) |
            (static_cast<unsigned>(data[offset+1]) << 8);
    };
    const auto axis = [&word](unsigned offset, bool invert) -> SHORT {
        int value = static_cast<int>(word(offset)) - 32768;
        if (invert) value = -value;
        if (value > 32767) value = 32767;
        return static_cast<SHORT>(value);
    };
    XINPUT_GAMEPAD result = {};
    constexpr WORD hats[] = {1,9,8,10,2,6,4,5};
    if (data[2] < 8) result.wButtons |= hats[data[2]];
    constexpr WORD buttons[] = {
        XINPUT_GAMEPAD_A, XINPUT_GAMEPAD_B, XINPUT_GAMEPAD_X, XINPUT_GAMEPAD_Y,
        XINPUT_GAMEPAD_LEFT_SHOULDER, XINPUT_GAMEPAD_RIGHT_SHOULDER,
        XINPUT_GAMEPAD_LEFT_THUMB, XINPUT_GAMEPAD_RIGHT_THUMB
    };
    for (unsigned bit = 0; bit < 8; ++bit)
        if (data[3] & (1u << bit)) result.wButtons |= buttons[bit];
    if (data[4] & 1) result.wButtons |= XINPUT_GAMEPAD_START;
    if (data[17] & 2) result.wButtons |= XINPUT_GAMEPAD_BACK;
    result.sThumbLX = axis(9,false); result.sThumbLY = axis(11,true);
    result.sThumbRX = axis(13,false); result.sThumbRY = axis(15,true);
    result.bLeftTrigger = static_cast<BYTE>(word(19) * 255u / 65535u);
    result.bRightTrigger = static_cast<BYTE>(word(21) * 255u / 65535u);
    pad = result;
    return true;
}
}
