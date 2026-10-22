#pragma once
#include <Windows.h>
#include <Xinput.h>
#include <cstdint>

// Original decoding from the public Sony report layouts, cross-checked against
// SDL_hidapi_ps4.c and Linux hid-playstation.c. No HID output/feature writes.
namespace RetroInput::DualShock4Reports {
// Windows Bluetooth HID advertises a maximum report buffer of 547 bytes.
inline bool IsSupportedReportBuffer(DWORD size) { return size >= 10 && size <= 1024; }
inline bool IsSupportedDevice(USHORT vendor, USHORT product) {
    return vendor == 0x054C && (product == 0x05C4 || product == 0x09CC);
}
inline std::uint32_t CrcByte(std::uint32_t crc, BYTE byte) {
    crc ^= byte;
    for (unsigned bit = 0; bit < 8; ++bit)
        crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320u : 0u);
    return crc;
}
inline bool ValidBluetoothCrc(const BYTE* data) {
    std::uint32_t crc = CrcByte(0xFFFFFFFFu, 0xA1);
    for (unsigned i = 0; i < 74; ++i) crc = CrcByte(crc, data[i]);
    const std::uint32_t expected = static_cast<std::uint32_t>(data[74]) |
        (static_cast<std::uint32_t>(data[75]) << 8) |
        (static_cast<std::uint32_t>(data[76]) << 16) |
        (static_cast<std::uint32_t>(data[77]) << 24);
    return ~crc == expected;
}
inline bool Decode(const BYTE* data, DWORD size, XINPUT_GAMEPAD& pad) {
    if (!data || !IsSupportedReportBuffer(size)) return false;
    unsigned base;
    if (data[0] == 0x01 && (size == 10 || size == 64 || size == 78 || size == 128 || size == 547)) base = 1;
    else if (data[0] == 0x11 && (size == 78 || size == 547) && (data[1] & 0x80) && ValidBluetoothCrc(data)) base = 3;
    else return false;
    const auto axis = [](BYTE value, bool invert) -> SHORT {
        int result = (static_cast<int>(value) - 128) * 257;
        if (invert) result = -result;
        if (result < -32768) result = -32768;
        if (result > 32767) result = 32767;
        return static_cast<SHORT>(result);
    };
    XINPUT_GAMEPAD decoded = {};
    decoded.sThumbLX = axis(data[base], false);
    decoded.sThumbLY = axis(data[base+1], true);
    decoded.sThumbRX = axis(data[base+2], false);
    decoded.sThumbRY = axis(data[base+3], true);
    const BYTE face = data[base+4], buttons = data[base+5];
    constexpr WORD hats[8] = {
        XINPUT_GAMEPAD_DPAD_UP, XINPUT_GAMEPAD_DPAD_UP | XINPUT_GAMEPAD_DPAD_RIGHT,
        XINPUT_GAMEPAD_DPAD_RIGHT, XINPUT_GAMEPAD_DPAD_RIGHT | XINPUT_GAMEPAD_DPAD_DOWN,
        XINPUT_GAMEPAD_DPAD_DOWN, XINPUT_GAMEPAD_DPAD_DOWN | XINPUT_GAMEPAD_DPAD_LEFT,
        XINPUT_GAMEPAD_DPAD_LEFT, XINPUT_GAMEPAD_DPAD_LEFT | XINPUT_GAMEPAD_DPAD_UP
    };
    const unsigned hat = face & 0x0F;
    if (hat < 8) decoded.wButtons |= hats[hat];
    if (face & 0x10) decoded.wButtons |= XINPUT_GAMEPAD_X;
    if (face & 0x20) decoded.wButtons |= XINPUT_GAMEPAD_A;
    if (face & 0x40) decoded.wButtons |= XINPUT_GAMEPAD_B;
    if (face & 0x80) decoded.wButtons |= XINPUT_GAMEPAD_Y;
    if (buttons & 0x01) decoded.wButtons |= XINPUT_GAMEPAD_LEFT_SHOULDER;
    if (buttons & 0x02) decoded.wButtons |= XINPUT_GAMEPAD_RIGHT_SHOULDER;
    if (buttons & 0x10) decoded.wButtons |= XINPUT_GAMEPAD_BACK; // Share
    if (buttons & 0x20) decoded.wButtons |= XINPUT_GAMEPAD_START; // Options
    if (buttons & 0x40) decoded.wButtons |= XINPUT_GAMEPAD_LEFT_THUMB;
    if (buttons & 0x80) decoded.wButtons |= XINPUT_GAMEPAD_RIGHT_THUMB;
    decoded.bLeftTrigger = data[base+7];
    decoded.bRightTrigger = data[base+8];
    pad = decoded;
    return true;
}
} // namespace RetroInput::DualShock4Reports
