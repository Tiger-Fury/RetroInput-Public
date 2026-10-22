#include "pch.h"
#include "Vx2.h"
#include "Core/ControllerActivity.h"
#include "Core/Vx2Report.h"
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
#include <string>

#pragma comment(lib, "hid.lib")
#pragma comment(lib, "setupapi.lib")

namespace RetroInput::Vx2Internal {
    std::mutex g_lock;
    Vx2Snapshot g_snapshot;
    std::atomic<bool> g_started{ false };
    // Diagnostics go to the current user's TEMP folder; no game files are modified.
    void Log(const char* format, ...) {
        wchar_t dir[MAX_PATH] = {};
        DWORD count = GetTempPathW(MAX_PATH, dir);
        if (!count || count >= MAX_PATH) return;
        wchar_t path[MAX_PATH] = {};
        if (swprintf_s(path, MAX_PATH, L"%lsSufferingVx2.log", dir) < 0) return;
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
    using RetroInput::Vx2Reports::Decode;
    using RetroInput::Vx2Reports::IsSupportedDevice;
    struct Descriptor {
        PHIDP_PREPARSED_DATA data = nullptr;
        RetroInput::Vx2Reports::Layout layout;
        ~Descriptor() { if(data) HidD_FreePreparsedData(data); }
    };

    HANDLE FindController(DWORD& reportLength, USHORT& productId, bool& bluetooth, Descriptor& descriptor) {
        reportLength = 0;
        productId = 0;
        bluetooth = false;
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
            // Only the identified VX-2 USB device/receiver is supported here.
            // Do not open unrelated keyboard, mouse, Sony or other HID devices.
            if ((!wcsstr(detail->DevicePath, L"25f0") && !wcsstr(detail->DevicePath, L"25F0")) ||
                (!wcsstr(detail->DevicePath, L"83c3") && !wcsstr(detail->DevicePath, L"83C3"))) continue;
            HANDLE h = CreateFileW(detail->DevicePath, GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                FILE_FLAG_OVERLAPPED, nullptr);
            if (h == INVALID_HANDLE_VALUE) {
                // Some Bluetooth HID interfaces are not readable; log only Vx2 paths.
                if (wcsstr(detail->DevicePath, L"vid_25f0") || wcsstr(detail->DevicePath, L"VID_25F0"))
                    Log("Vx2 HID open failed error=%lu path=%ls", GetLastError(), detail->DevicePath);
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
                    caps.UsagePage == 0x01 && (caps.Usage == 0x04 || caps.Usage == 0x05);
                if (valid) candidateLength = caps.InputReportByteLength;
                if(valid && RetroInput::Vx2Reports::IsSupportedReportBuffer(candidateLength)) {
                    std::vector<HIDP_VALUE_CAPS> values(caps.NumberInputValueCaps);
                    std::vector<HIDP_BUTTON_CAPS> buttons(caps.NumberInputButtonCaps);
                    USHORT nv=caps.NumberInputValueCaps, nb=caps.NumberInputButtonCaps;
                    valid=nv && nb && HidP_GetValueCaps(HidP_Input,values.data(),&nv,preparsed)==HIDP_STATUS_SUCCESS &&
                        HidP_GetButtonCaps(HidP_Input,buttons.data(),&nb,preparsed)==HIDP_STATUS_SUCCESS;
                    if(valid) {
                        descriptor.layout=RetroInput::Vx2Reports::BuildLayout(values.data(),nv,buttons.data(),nb);
                        valid=descriptor.layout.valid;
                        for(USHORT i=0;i<nv;++i) {
                            const auto& c=values[i];
                            Log("Value cap page=%04X usage=%04X..%04X id=%u bits=%u min=%ld max=%ld index=%u",
                                c.UsagePage,c.IsRange?c.Range.UsageMin:c.NotRange.Usage,
                                c.IsRange?c.Range.UsageMax:c.NotRange.Usage,c.ReportID,c.BitSize,c.LogicalMin,c.LogicalMax,
                                c.IsRange?c.Range.DataIndexMin:c.NotRange.DataIndex);
                        }
                        for(USHORT i=0;i<nb;++i) {
                            const auto& c=buttons[i];
                            Log("Button cap page=%04X usage=%04X..%04X id=%u link=%u",c.UsagePage,
                                c.IsRange?c.Range.UsageMin:c.NotRange.Usage,c.IsRange?c.Range.UsageMax:c.NotRange.Usage,
                                c.ReportID,c.LinkCollection);
                        }
                    }
                } else valid=false;
                if(valid) descriptor.data=preparsed;
                else HidD_FreePreparsedData(preparsed);
            }
            else valid = false;
            if (IsSupportedDevice(attr.VendorID, attr.ProductID))
                Log("Vx2 HID candidate pid=%04X valid=%d inputReportBytes=%lu path=%ls",
                    attr.ProductID, valid ? 1 : 0, candidateLength, detail->DevicePath);
            if (valid && RetroInput::Vx2Reports::IsSupportedReportBuffer(candidateLength)) {
                found = h;
                reportLength = candidateLength;
                productId = attr.ProductID;
                bluetooth = wcsstr(detail->DevicePath, L"{00001124-") != nullptr;
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
            bool bluetooth = false;
            Descriptor descriptor;
            HANDLE device = FindController(reportLength, productId, bluetooth, descriptor);
            if (device == INVALID_HANDLE_VALUE) {
                DWORD now = GetTickCount();
                if (lastMissingLog == 0 || now - lastMissingLog >= 10000) {
                    Log("No suitable Vx2 HID interface; retrying");
                    lastMissingLog = now;
                }
                Sleep(1500); continue;
            }
            Log("Opened Vx2 HID; pid=%04X inputReportBytes=%lu bluetooth=%d", productId, reportLength, bluetooth ? 1 : 0);

            HANDLE event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
            if (!event) { CloseHandle(device); Sleep(1000); continue; }

            // ReadFile must use the HID device's actual report length.
            // USB state fields are parsed through the advertised HID descriptor.
            std::vector<BYTE> report(reportLength);
            {
                std::lock_guard<std::mutex> lock(g_lock);
                g_snapshot = {}; // Set connected only after a valid report.
            }

            bool reportedFirst = false;
            bool loggedUnknown[256] = {};
            bool loggedActivity = false;
            bool loggedState = false;
            for (;;) {
                OVERLAPPED ov = {};
                ov.hEvent = event;
                ResetEvent(event);
                DWORD received = 0;
                BOOL ok = ReadFile(device, report.data(), reportLength, &received, &ov);
                if (!ok && GetLastError() == ERROR_IO_PENDING) {
                    DWORD wait = WaitForSingleObject(event, 10000);
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
                    char hex[193] = {};
                    for (DWORD i=0; i<received && i<64; ++i)
                        sprintf_s(hex+i*3, sizeof(hex)-i*3, "%02X ", report[i]);
                    Log("First report hex=%s",hex);
                    Log("First HID report: received=%lu reportID=0x%02X firstBytes=%02X %02X %02X %02X",
                        received, received ? report[0] : 0, received > 1 ? report[1] : 0,
                        received > 2 ? report[2] : 0, received > 3 ? report[3] : 0,
                        received > 4 ? report[4] : 0);
                    reportedFirst = true;
                }

                XINPUT_GAMEPAD pad = {};
                if (!Decode(descriptor.data, descriptor.layout, report.data(), received, pad)) {
                    if (received && !loggedUnknown[report[0]]) {
                        loggedUnknown[report[0]] = true;
                        Log("Unsupported report: received=%lu reportID=0x%02X", received, report[0]);
                    }
                    continue;
                }
                if(!loggedState) {
                    Log("Decoded state: buttons=%04X axes=%d,%d,%d,%d triggers=%u,%u",pad.wButtons,
                        pad.sThumbLX,pad.sThumbLY,pad.sThumbRX,pad.sThumbRY,pad.bLeftTrigger,pad.bRightTrigger);
                    loggedState=true;
                }
                std::lock_guard<std::mutex> lock(g_lock);
                g_snapshot.connected = true;
                g_snapshot.gamepad = pad;
                g_snapshot.productId = productId;
                g_snapshot.bluetooth = bluetooth;
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

bool StartVx2Reader() {
    using namespace RetroInput::Vx2Internal;
    if (g_started.exchange(true)) return true;
    HANDLE thread = CreateThread(nullptr, 0, Reader, nullptr, 0, nullptr);
    if (!thread) { g_started = false; return false; }
    CloseHandle(thread);
    return true;
}

Vx2Snapshot GetVx2Snapshot() {
    using namespace RetroInput::Vx2Internal;
    std::lock_guard<std::mutex> lock(g_lock);
    return g_snapshot;
}

void ResetVx2Activity() {
    using namespace RetroInput::Vx2Internal;
    std::lock_guard<std::mutex> lock(g_lock);
    g_snapshot.lastActivity = 0;
}
