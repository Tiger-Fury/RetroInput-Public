#include "pch.h"
#include "DualShock4.h"
#include "Core/ControllerActivity.h"
#include "Core/DualShock4Report.h"
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

namespace RetroInput::DualShock4Internal {
    std::mutex g_lock;
    DualShock4Snapshot g_snapshot;
    std::atomic<bool> g_started{ false };
    // Diagnostics go to the current user's TEMP folder; no game files are modified.
    void Log(const char* format, ...) {
        wchar_t dir[MAX_PATH] = {};
        DWORD count = GetTempPathW(MAX_PATH, dir);
        if (!count || count >= MAX_PATH) return;
        wchar_t path[MAX_PATH] = {};
        if (swprintf_s(path, MAX_PATH, L"%sSufferingDualShock4.log", dir) < 0) return;
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
    using RetroInput::DualShock4Reports::Decode;
    using RetroInput::DualShock4Reports::IsSupportedDevice;

    HANDLE FindController(DWORD& reportLength, USHORT& productId) {
        reportLength = 0;
        productId = 0;
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
            bool valid = HidD_GetAttributes(h, &attr) && IsSupportedDevice(attr.VendorID, attr.ProductID);
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
            if (IsSupportedDevice(attr.VendorID, attr.ProductID))
                Log("Sony HID candidate pid=%04X valid=%d inputReportBytes=%lu path=%ls",
                    attr.ProductID, valid ? 1 : 0, candidateLength, detail->DevicePath);
            if (valid && RetroInput::DualShock4Reports::IsSupportedReportBuffer(candidateLength)) {
                found = h;
                reportLength = candidateLength;
                productId = attr.ProductID;
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
            USHORT productId = 0;
            HANDLE device = FindController(reportLength, productId);
            if (device == INVALID_HANDLE_VALUE) {
                DWORD now = GetTickCount();
                if (lastMissingLog == 0 || now - lastMissingLog >= 10000) {
                    Log("No suitable DualShock4 HID interface; retrying");
                    lastMissingLog = now;
                }
                Sleep(1500); continue;
            }
            Log("Opened DualShock4 HID; pid=%04X inputReportBytes=%lu", productId, reportLength);

            HANDLE event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
            if (!event) { CloseHandle(device); Sleep(1000); continue; }

            // ReadFile must use the HID device's actual report length.
            // DualShock4 USB normally reports 64 bytes; Bluetooth full
            // reports use 78 bytes inside a Windows HID buffer that may be 547 bytes.
            std::vector<BYTE> report(reportLength);
            {
                std::lock_guard<std::mutex> lock(g_lock);
                g_snapshot = {}; // Set connected only after a valid report.
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
                    DWORD wait = WaitForSingleObject(event, reportLength > 64 ? 10000 : 2000);
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
                g_snapshot.productId = productId;
                g_snapshot.bluetooth = report[0] == 0x11 || reportLength == 10 || reportLength > 64;
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
                g_snapshot = {};
            }
            Log("Closed HID connection; scanning again");
            CloseHandle(event);
            CloseHandle(device);
            Sleep(750);
        }
    }
}

bool StartDualShock4Reader() {
    using namespace RetroInput::DualShock4Internal;
    if (g_started.exchange(true)) return true;
    HANDLE thread = CreateThread(nullptr, 0, Reader, nullptr, 0, nullptr);
    if (!thread) { g_started = false; return false; }
    CloseHandle(thread);
    return true;
}

DualShock4Snapshot GetDualShock4Snapshot() {
    using namespace RetroInput::DualShock4Internal;
    std::lock_guard<std::mutex> lock(g_lock);
    return g_snapshot;
}

void ResetDualShock4Activity() {
    using namespace RetroInput::DualShock4Internal;
    std::lock_guard<std::mutex> lock(g_lock);
    g_snapshot.lastActivity = 0;
}
