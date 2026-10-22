#include "pch.h"
#include <Windows.h>
#include <cstdint>
#include <cstring>

// TTB x86 input sample hook. This adapter has no text/render hooks yet.
namespace {
constexpr uintptr_t HookAddress = 0x0040DD18;
constexpr BYTE ExpectedBytes[] = { 0x8B, 0x53, 0x18, 0x8B, 0x43, 0x10 };
bool installed = false;

bool CompatibleHost()
{
    wchar_t path[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (!length || length >= MAX_PATH) return false;
    const wchar_t* name = wcsrchr(path, L'\\');
    if (_wcsicmp(name ? name + 1 : path, L"SufferingTTB.exe") != 0) return false;
    const auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    if (base != 0x00400000) return false;
    const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return false;
    const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(base + dos->e_lfanew);
    return nt->Signature == IMAGE_NT_SIGNATURE &&
        nt->FileHeader.Machine == IMAGE_FILE_MACHINE_I386 &&
        nt->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC &&
        nt->OptionalHeader.SizeOfImage == 3538944 &&
        nt->OptionalHeader.AddressOfEntryPoint == 0x001D8426 &&
        std::memcmp(reinterpret_cast<const void*>(HookAddress), ExpectedBytes, sizeof(ExpectedBytes)) == 0;
}
}

extern "C" { DWORD g_TTBReturnAddress = 0x0040DD1E; }
void __stdcall InjectControllerInput(DWORD record, float* state);

__declspec(naked) void TTBInputHook()
{
    __asm {
        pushfd
        pushad
        // The input hook calls C++ floating-point code. Preserve the game's
        // x87/SSE state and floating-point control settings around that call.
        mov ebp, esp
        sub esp, 528
        and esp, 0FFFFFFF0h
        fxsave [esp]
        fninit
        push esi
        push ebx
        call InjectControllerInput
        fxrstor [esp]
        mov esp, ebp
        popad
        popfd
        mov edx, [ebx + 18h]
        mov eax, [ebx + 10h]
        jmp dword ptr[g_TTBReturnAddress]
    }
}

extern "C" __declspec(dllexport) BOOL StartControllerHook()
{
    if (installed) return TRUE;
    if (!CompatibleHost()) {
        OutputDebugStringW(L"RetroInput TTB: incompatible executable or input hook bytes.\n");
        return FALSE;
    }
    auto target = reinterpret_cast<BYTE*>(HookAddress);
    DWORD oldProtection = 0;
    if (!VirtualProtect(target, sizeof(ExpectedBytes), PAGE_EXECUTE_READWRITE, &oldProtection)) return FALSE;
    const auto displacement = static_cast<std::int32_t>(
        reinterpret_cast<uintptr_t>(&TTBInputHook) - (HookAddress + 5));
    BYTE patch[] = { 0xE9, 0, 0, 0, 0, 0x90 };
    std::memcpy(patch + 1, &displacement, sizeof(displacement));
    std::memcpy(target, patch, sizeof(patch));
    FlushInstructionCache(GetCurrentProcess(), target, sizeof(patch));
    DWORD unused;
    VirtualProtect(target, sizeof(patch), oldProtection, &unused);
    installed = true;
    OutputDebugStringW(L"RetroInput TTB: controller input hook installed.\n");
    return TRUE;
}
