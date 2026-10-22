#include "pch.h"
#include "DualSense.h"
#include "Core/ControllerActivity.h"
#include <Windows.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <setupapi.h>
#include <atomic>
#include <mutex>
#include <vector>
#include <cstdio>
#include <cstdarg>
#include <cstring>

#pragma comment(lib, "hid.lib")
#pragma comment(lib, "setupapi.lib")

namespace {
    std::mutex g_lock;
    DualSenseSnapshot g_snapshot;
    std::atomic<bool> g_started{ false };
    // Diagnostics go to the current user's TEMP folder; no game files are modified.
    void Log(const char* format, ...) {
        wchar_t dir[MAX_PATH] = {};
        DWORD count = GetTempPathW(MAX_PATH, dir);
        if (!count || count >= MAX_PATH) return;
        wchar_t path[MAX_PATH] = {};
        if (swprintf_s(path, MAX_PATH, L"%sSufferingDualSense.log", dir) < 0) return;
        FILE* f = nullptr;
        if (_wfopen_s(&f, path, L"a") != 0 || !f) return;
        SYSTEMTIME t = {};
        GetLocalTime(&t);
        fprintf(f, "%02u:%02u:%02u.%03u ", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
        va_list args;
        va_start(args, format);
        vfprintf(f, format, args);
        va_end(args);
        fputc('\n', f);
        fclose(f);
    }
    constexpr USHORT SONY = 0x054C;
    constexpr USHORT DUALSENSE = 0x0CE6;
    constexpr USHORT DUALSENSE_EDGE = 0x0DF2;

    // Report 0x01/78 uses the compact Bluetooth input layout on this HID
    // interface: sticks at 1..4, face/hat at 5, buttons at 6,
    // analog L2/R2 at 8/9. USB 0x01/64 and full BT 0x31 use their standard layouts.
    bool Decode(const BYTE* data, DWORD size, XINPUT_GAMEPAD& pad) {
        if (size < 12) return false;
        const bool compactBT = data[0] == 0x01 && size == 78;
        size_t base;
        if (data[0] == 0x31 && size >= 13) base = 2;
        else if (data[0] == 0x01 && (size == 64 || size == 78)) base = 1;
        else return false;
        const BYTE lx = data[base], ly = data[base + 1];
        const BYTE rx = data[base + 2], ry = data[base + 3];
        const BYTE l2 = compactBT ? data[8] : data[base + 4];
        const BYTE r2 = compactBT ? data[9] : data[base + 5];
        const BYTE face = compactBT ? data[5] : data[base + 7];
        const BYTE buttons = compactBT ? data[6] : data[base + 8];
        auto axis = [](BYTE b, bool invert) -> SHORT {
            int v = (static_cast<int>(b) - 128) * 257;
            if (invert) v = -v;
            if (v < -32768) v = -32768;
            if (v > 32767) v = 32767;
            return static_cast<SHORT>(v);
            };
        pad = {};
        pad.sThumbLX = axis(lx, false);
        pad.sThumbLY = axis(ly, true);
        pad.sThumbRX = axis(rx, false);
        pad.sThumbRY = axis(ry, true);
        pad.bLeftTrigger = l2;
        pad.bRightTrigger = r2;
        switch (face & 0x0f) {
        case 0: pad.wButtons |= XINPUT_GAMEPAD_DPAD_UP; break;
        case 1: pad.wButtons |= XINPUT_GAMEPAD_DPAD_UP | XINPUT_GAMEPAD_DPAD_RIGHT; break;
        case 2: pad.wButtons |= XINPUT_GAMEPAD_DPAD_RIGHT; break;
        case 3: pad.wButtons |= XINPUT_GAMEPAD_DPAD_RIGHT | XINPUT_GAMEPAD_DPAD_DOWN; break;
        case 4: pad.wButtons |= XINPUT_GAMEPAD_DPAD_DOWN; break;
        case 5: pad.wButtons |= XINPUT_GAMEPAD_DPAD_DOWN | XINPUT_GAMEPAD_DPAD_LEFT; break;
        case 6: pad.wButtons |= XINPUT_GAMEPAD_DPAD_LEFT; break;
        case 7: pad.wButtons |= XINPUT_GAMEPAD_DPAD_LEFT | XINPUT_GAMEPAD_DPAD_UP; break;
        }
        if (face & 0x10) pad.wButtons |= XINPUT_GAMEPAD_X; // Square
        if (face & 0x20) pad.wButtons |= XINPUT_GAMEPAD_A; // Cross
        if (face & 0x40) pad.wButtons |= XINPUT_GAMEPAD_B; // Circle
        if (face & 0x80) pad.wButtons |= XINPUT_GAMEPAD_Y; // Triangle
        if (buttons & 0x01) pad.wButtons |= XINPUT_GAMEPAD_LEFT_SHOULDER;
        if (buttons & 0x02) pad.wButtons |= XINPUT_GAMEPAD_RIGHT_SHOULDER;
        if (buttons & 0x10) pad.wButtons |= XINPUT_GAMEPAD_BACK; // Create
        if (buttons & 0x20) pad.wButtons |= XINPUT_GAMEPAD_START; // Options
        if (buttons & 0x40) pad.wButtons |= XINPUT_GAMEPAD_LEFT_THUMB;  // L3
        if (buttons & 0x80) pad.wButtons |= XINPUT_GAMEPAD_RIGHT_THUMB; // R3
        return true;
    }

    HANDLE FindController(DWORD& reportLength) {
        reportLength = 0;
        GUID guid;
        HidD_GetHidGuid(&guid);
        HDEVINFO info = SetupDiGetClassDevsW(&guid, nullptr, nullptr,
            DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
        if (info == INVALID_HANDLE_VALUE) { Log("HID enumeration failed error=%lu", GetLastError()); return INVALID_HANDLE_VALUE; }
        HANDLE found = INVALID_HANDLE_VALUE;
        for (DWORD index = 0; ; ++index) {
            SP_DEVICE_INTERFACE_DATA item = {};
            item.cbSize = sizeof(item);
            if (!SetupDiEnumDeviceInterfaces(info, nullptr, &guid, index, &item)) break;
            DWORD needed = 0;
            SetupDiGetDeviceInterfaceDetailW(info, &item, nullptr, 0, &needed, nullptr);
            if (!needed) continue;
            std::vector<BYTE> memory(needed);
            auto* detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(memory.data());
            detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
            if (!SetupDiGetDeviceInterfaceDetailW(info, &item, detail, needed, nullptr, nullptr)) continue;
            HANDLE h = CreateFileW(detail->DevicePath, GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                FILE_FLAG_OVERLAPPED, nullptr);
            if (h == INVALID_HANDLE_VALUE) {
                // Some Bluetooth HID interfaces are not readable; log only Sony paths.
                if (wcsstr(detail->DevicePath, L"vid_054c") || wcsstr(detail->DevicePath, L"VID_054C"))
                    Log("Sony HID open failed error=%lu path=%ls", GetLastError(), detail->DevicePath);
                continue;
            }
            HIDD_ATTRIBUTES attr = {};
            attr.Size = sizeof(attr);
            bool valid = HidD_GetAttributes(h, &attr) && attr.VendorID == SONY &&
                (attr.ProductID == DUALSENSE || attr.ProductID == DUALSENSE_EDGE);
            PHIDP_PREPARSED_DATA preparsed = nullptr;
            DWORD candidateLength = 0;
            if (valid && HidD_GetPreparsedData(h, &preparsed)) {
                HIDP_CAPS caps = {};
                valid = HidP_GetCaps(preparsed, &caps) == HIDP_STATUS_SUCCESS &&
                    caps.UsagePage == 0x01 && caps.Usage == 0x05;
                if (valid) candidateLength = caps.InputReportByteLength;
                HidD_FreePreparsedData(preparsed);
            }
            else valid = false;
            if (attr.VendorID == SONY && (attr.ProductID == DUALSENSE || attr.ProductID == DUALSENSE_EDGE))
                Log("Sony HID candidate pid=%04X valid=%d inputReportBytes=%lu path=%ls",
                    attr.ProductID, valid ? 1 : 0, candidateLength, detail->DevicePath);
            if (valid && candidateLength >= 64 && candidateLength <= 512) {
                found = h;
                reportLength = candidateLength;
                break;
            }
            CloseHandle(h);
        }
        SetupDiDestroyDeviceInfoList(info);
        return found;
    }

    DWORD WINAPI Reader(LPVOID) {
        Log("Reader thread started");
        DWORD lastMissingLog = 0;
        for (;;) {
            DWORD reportLength = 0;
            HANDLE device = FindController(reportLength);
            if (device == INVALID_HANDLE_VALUE) {
                DWORD now = GetTickCount();
                if (lastMissingLog == 0 || now - lastMissingLog >= 10000) {
                    Log("No suitable DualSense HID interface; retrying");
                    lastMissingLog = now;
                }
                Sleep(1500); continue;
            }
            Log("Opened DualSense HID; inputReportBytes=%lu", reportLength);

            HANDLE event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
            if (!event) { CloseHandle(device); Sleep(1000); continue; }

            // ReadFile must use the HID device's actual report length.
            // DualSense USB normally reports 64 bytes; Bluetooth full
            // reports normally use 78 bytes.
            std::vector<BYTE> report(reportLength);
            {
                std::lock_guard<std::mutex> lock(g_lock);
                g_snapshot.connected = false; // Set true only after a valid report.
                g_snapshot.gamepad = {};
                g_snapshot.lastActivity = 0;
            }

            bool reportedFirst = false;
            bool loggedUnknown[256] = {};
            bool loggedActivity = false;
            for (;;) {
                OVERLAPPED ov = {};
                ov.hEvent = event;
                ResetEvent(event);
                DWORD received = 0;
                BOOL ok = ReadFile(device, report.data(), reportLength, &received, &ov);
                if (!ok && GetLastError() == ERROR_IO_PENDING) {
                    DWORD wait = WaitForSingleObject(event, reportLength == 78 ? 10000 : 2000);
                    if (wait == WAIT_OBJECT_0) {
                        ok = GetOverlappedResult(device, &ov, &received, FALSE);
                    }
                    else {
                        // Never close the event, device, or return while the
                        // stack-allocated OVERLAPPED operation is still pending.
                        // Unplugging a controller can leave ReadFile pending.
                        CancelIoEx(device, &ov);
                        DWORD ignored = 0;
                        GetOverlappedResult(device, &ov, &ignored, TRUE);
                        Log("Read timed out; cancelled pending read, rescan device");
                        break;
                    }
                }
                if (!ok) { Log("Read failed error=%lu; rescan device", GetLastError()); break; }
                if (!reportedFirst) {
                    Log("First HID report: received=%lu reportID=0x%02X firstBytes=%02X %02X %02X %02X",
                        received, received ? report[0] : 0, received > 1 ? report[1] : 0,
                        received > 2 ? report[2] : 0, received > 3 ? report[3] : 0,
                        received > 4 ? report[4] : 0);
                    reportedFirst = true;
                }

                XINPUT_GAMEPAD pad = {};
                if (!Decode(report.data(), received, pad)) {
                    if (received && !loggedUnknown[report[0]]) {
                        loggedUnknown[report[0]] = true;
                        Log("Unsupported report: received=%lu reportID=0x%02X", received, report[0]);
                    }
                    continue;
                }
                std::lock_guard<std::mutex> lock(g_lock);
                g_snapshot.connected = true;
                g_snapshot.gamepad = pad;
                if (RetroInput::Controllers::HasActivity(pad)) {
                    g_snapshot.lastActivity = GetTickCount();
                    if (!loggedActivity) {
                        Log("Decoded active controller input: reportID=0x%02X buttons=0x%04X lx=%d ly=%d",
                            report[0], pad.wButtons, pad.sThumbLX, pad.sThumbLY);
                        loggedActivity = true;
                    }
                }
            }

            {
                std::lock_guard<std::mutex> lock(g_lock);
                g_snapshot.connected = false;
                g_snapshot.gamepad = {};
                g_snapshot.lastActivity = 0;
            }
            Log("Closed HID connection; scanning again");
            CloseHandle(event);
            CloseHandle(device);
            Sleep(750);
        }
    }
}

bool StartDualSenseReader() {
    if (g_started.exchange(true)) return true;
    HANDLE thread = CreateThread(nullptr, 0, Reader, nullptr, 0, nullptr);
    if (!thread) { g_started = false; return false; }
    CloseHandle(thread);
    return true;
}

DualSenseSnapshot GetDualSenseSnapshot() {
    std::lock_guard<std::mutex> lock(g_lock);
    return g_snapshot;
}

void ResetDualSenseActivity() {
    std::lock_guard<std::mutex> lock(g_lock);
    g_snapshot.lastActivity = 0;
}
