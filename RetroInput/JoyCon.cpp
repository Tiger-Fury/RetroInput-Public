#include "pch.h"
#include "JoyCon.h"
#include "Core/JoyConReport.h"
#include "Core/ControllerActivity.h"
#include <hidsdi.h>
#include <hidpi.h>
#include <setupapi.h>
#include <atomic>
#include <mutex>
#include <vector>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cwctype>
#include <string>
#pragma comment(lib, "hid.lib")
#pragma comment(lib, "setupapi.lib")

namespace RetroInput::JoyConInternal {
std::mutex lock;
JoyConReports::Half halves[2];
std::atomic<bool> started[2] = {};
void Log(unsigned side, const char* format, ...) {
    wchar_t dir[MAX_PATH] = {}, path[MAX_PATH] = {};
    const DWORD length = GetTempPathW(MAX_PATH, dir);
    if (!length || length >= MAX_PATH ||
        swprintf_s(path,L"%lsRetroInputJoyCon.log",dir)<0) return;
    FILE* file = nullptr;
    if (_wfopen_s(&file,path,L"a") || !file) return;
    fprintf(file,"%lu %s ",GetTickCount(),side ? "Right" : "Left");
    va_list args; va_start(args,format); vfprintf(file,format,args); va_end(args);
    fputc('\n',file); fclose(file);
}
// Each half owns one handle, one read/write worker and one snapshot.
struct Device {
    HANDLE handle = INVALID_HANDLE_VALUE, event = nullptr;
    DWORD inputLength = 0, outputLength = 0;
    BYTE sequence = 0;
    ~Device() {
        if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle);
        if (event) CloseHandle(event);
    }
    bool Transfer(bool write, BYTE* data, DWORD length, DWORD& received, DWORD timeout) {
        OVERLAPPED ov = {}; ov.hEvent = event; ResetEvent(event);
        BOOL ok = write ? WriteFile(handle,data,length,&received,&ov) :
            ReadFile(handle,data,length,&received,&ov);
        if (!ok && GetLastError() == ERROR_IO_PENDING) {
            if (WaitForSingleObject(event,timeout) == WAIT_OBJECT_0)
                ok = GetOverlappedResult(handle,&ov,&received,FALSE);
            else {
                CancelIoEx(handle,&ov);
                DWORD ignored = 0;
                GetOverlappedResult(handle,&ov,&ignored,TRUE);
                return false;
            }
        }
        return ok != FALSE;
    }
    bool Command(BYTE command, const BYTE* data, unsigned size, std::vector<BYTE>& reply) {
        for (unsigned attempt=0; attempt<3; ++attempt) {
            std::vector<BYTE> packet(outputLength);
            packet[0]=1; packet[1]=sequence++ & 15;
            const BYTE neutral[8]={0,1,0x40,0x40,0,1,0x40,0x40};
            memcpy(packet.data()+2,neutral,sizeof(neutral));
            packet[10]=command;
            if (size) memcpy(packet.data()+11,data,size);
            DWORD count=0;
            if (!Transfer(true,packet.data(),outputLength,count,500) || count!=outputLength) continue;
            const DWORD begin=GetTickCount();
            while (DWORD(GetTickCount()-begin)<700) {
                reply.assign(inputLength,0);
                if (!Transfer(false,reply.data(),inputLength,count,200)) break;
                reply.resize(count);
                if (count<15 || reply[0]!=0x21 || !(reply[13]&0x80) || reply[14]!=command) continue;
                if (command==0x10 && (count<20 || memcmp(reply.data()+15,data,5))) continue;
                return true;
            }
        }
        return false;
    }
    bool SPI(unsigned address, BYTE* data, BYTE length) {
        const BYTE params[5]={BYTE(address),BYTE(address>>8),BYTE(address>>16),BYTE(address>>24),length};
        std::vector<BYTE> reply;
        if (!Command(0x10,params,5,reply) || reply.size()<20u+length) return false;
        memcpy(data,reply.data()+20,length); return true;
    }
};
bool Open(unsigned side, Device& device) {
    GUID guid; HidD_GetHidGuid(&guid);
    HDEVINFO info=SetupDiGetClassDevsW(&guid,nullptr,nullptr,DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);
    if (info==INVALID_HANDLE_VALUE) return false;
    for (DWORD i=0;;++i) {
        SP_DEVICE_INTERFACE_DATA item={}; item.cbSize=sizeof(item);
        if (!SetupDiEnumDeviceInterfaces(info,nullptr,&guid,i,&item)) break;
        DWORD needed=0;
        SetupDiGetDeviceInterfaceDetailW(info,&item,nullptr,0,&needed,nullptr);
        if (!needed) continue;
        std::vector<BYTE> buffer(needed);
        auto detail=reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(buffer.data());
        detail->cbSize=sizeof(*detail);
        if (!SetupDiGetDeviceInterfaceDetailW(info,&item,detail,needed,nullptr,nullptr)) continue;
        std::wstring path(detail->DevicePath);
        for (auto& c:path) c=static_cast<wchar_t>(towlower(c));
        if (path.find(L"057e")==std::wstring::npos ||
            path.find(side?L"2007":L"2006")==std::wstring::npos) continue;
        HANDLE h=CreateFileW(detail->DevicePath,GENERIC_READ|GENERIC_WRITE,
            FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,nullptr);
        if (h==INVALID_HANDLE_VALUE) {Log(side,"Open failed error=%lu",GetLastError());continue;}
        HIDD_ATTRIBUTES attributes={};attributes.Size=sizeof(attributes);
        PHIDP_PREPARSED_DATA preparsed=nullptr; HIDP_CAPS caps={};
        bool valid=HidD_GetAttributes(h,&attributes) && attributes.VendorID==0x057E &&
            attributes.ProductID==(side?0x2007:0x2006) && HidD_GetPreparsedData(h,&preparsed);
        if (valid) valid=HidP_GetCaps(preparsed,&caps)==HIDP_STATUS_SUCCESS &&
            caps.UsagePage==1 && (caps.Usage==4 || caps.Usage==5) &&
            caps.InputReportByteLength>=49 && caps.InputReportByteLength<=1024 &&
            caps.OutputReportByteLength>=49 && caps.OutputReportByteLength<=1024;
        if (preparsed) HidD_FreePreparsedData(preparsed);
        if (!valid) {CloseHandle(h);continue;}
        device.event=CreateEventW(nullptr,TRUE,FALSE,nullptr);
        if (!device.event) {CloseHandle(h);break;}
        device.handle=h; device.inputLength=caps.InputReportByteLength;
        device.outputLength=caps.OutputReportByteLength;
        Log(side,"Opened HID inputBytes=%lu outputBytes=%lu",device.inputLength,device.outputLength);
        break;
    }
    SetupDiDestroyDeviceInfoList(info);
    return device.handle!=INVALID_HANDLE_VALUE;
}
bool Calibrate(unsigned side, Device& device, JoyConReports::Calibration& calibration) {
    BYTE user[11]={}, factory[9]={};
    if (device.SPI(side?0x801B:0x8010,user,sizeof(user)) && user[0]==0xB2 && user[1]==0xA1 &&
        JoyConReports::ReadCalibration(user+2,9,side!=0,calibration)) {
        Log(side,"Loaded user stick calibration");return true;
    }
    if (device.SPI(side?0x6046:0x603D,factory,sizeof(factory)) &&
        JoyConReports::ReadCalibration(factory,9,side!=0,calibration)) {
        Log(side,"Loaded factory stick calibration");return true;
    }
    Log(side,"No valid stick calibration; retrying connection");return false;
}
DWORD WINAPI Reader(LPVOID parameter) {
    const unsigned side=static_cast<unsigned>(reinterpret_cast<ULONG_PTR>(parameter));
    Log(side,"Reader started"); DWORD lastMissing=0;
    for (;;) {
        {
            Device device;
            if (Open(side,device)) {
                JoyConReports::Calibration calibration;
                const BYTE mode=0x30; std::vector<BYTE> reply;
                if (Calibrate(side,device,calibration) && device.Command(3,&mode,1,reply)) {
                    Log(side,"Full report mode enabled");
                    bool first=true, firstActive=true;
                    for (;;) {
                        std::vector<BYTE> report(device.inputLength);DWORD count=0;
                        if (!device.Transfer(false,report.data(),device.inputLength,count,1500)) {
                            Log(side,"Read failed or timed out; rescan");break;
                        }
                        XINPUT_GAMEPAD pad={};
                        if (!JoyConReports::Decode(report.data(),count,side!=0,calibration,pad)) continue;
                        const DWORD now=GetTickCount();
                        const bool active=Controllers::HasActivity(pad);
                        {
                            std::lock_guard<std::mutex> guard(lock);
                            auto& half=halves[side];half.connected=true;half.received=now;half.pad=pad;
                            if (active) half.activity=now;
                        }
                        if (first) {Log(side,"First calibrated full input report received");first=false;}
                        if (active && firstActive) {Log(side,"First active input buttons=%04X",pad.wButtons);firstActive=false;}
                    }
                } else Log(side,"Initialization failed; retrying");
            } else if (!lastMissing || DWORD(GetTickCount()-lastMissing)>=10000) {
                Log(side,"No available Joy-Con interface; retrying");lastMissing=GetTickCount();
            }
        }
        {std::lock_guard<std::mutex> guard(lock);halves[side]={};}
        Sleep(1000);
    }
}
} // namespace RetroInput::JoyConInternal
bool StartJoyConReader() {
    using namespace RetroInput::JoyConInternal;
    bool ok=true;
    for (unsigned side=0;side<2;++side) {
        if (started[side].exchange(true)) continue;
        HANDLE thread=CreateThread(nullptr,0,Reader,reinterpret_cast<LPVOID>(ULONG_PTR(side)),0,nullptr);
        if (thread) CloseHandle(thread);
        else {started[side]=false;ok=false;}
    }
    return ok;
}
JoyConSnapshot GetJoyConSnapshot() {
    using namespace RetroInput::JoyConInternal;
    std::lock_guard<std::mutex> guard(lock);JoyConSnapshot snapshot;
    snapshot.connected=RetroInput::JoyConReports::Combine(halves[0],halves[1],GetTickCount(),
        snapshot.gamepad,snapshot.lastActivity);
    return snapshot;
}
void ResetJoyConActivity() {
    using namespace RetroInput::JoyConInternal;
    std::lock_guard<std::mutex> guard(lock);
    halves[0].activity=halves[1].activity=0;
}
