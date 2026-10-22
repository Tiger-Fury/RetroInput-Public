#include "pch.h"
#include <Windows.h>
#include <cstdint>
#include <cstring>

// The Suffering (2004) x86-specific game hook.
// Preserve original instruction bytes and call convention.
static constexpr uintptr_t HOOK_ADDRESS = 0x0040CAEB;

static BYTE originalBytes[6] = {};
static bool hookInstalled = false;

extern "C" {
    DWORD g_ReturnAddress = 0x0040CAF1;
}

void __stdcall InjectControllerInput(DWORD deviceRecord, float* state);

// Assembly hook. Only works in x86 builds.

__declspec(naked) void InputHook()
{
    __asm
    {
        // Save registers and flags.
        pushfd
        pushad

        // Preserve floating-point state around movement and camera calculations.
        mov ebp, esp
        sub esp, 528
        and esp, 0FFFFFFF0h
        fxsave [esp]
        fninit
        // Pass the current input buffer and device record.
        push esi
        push ebx
        call InjectControllerInput

        fxrstor [esp]
        mov esp, ebp
        // Restore registers and flags.
        popad
        popfd

        // Execute the original overwritten instructions.
        mov edx, [ebx + 0x1C]
        mov eax, [ebx + 0x14]

        // Return without changing any registers.
        jmp dword ptr[g_ReturnAddress]
    }
}

bool InstallHook()
{
    if (hookInstalled)
        return true;

    BYTE* target = reinterpret_cast<BYTE*>(HOOK_ADDRESS);

    DWORD oldProtection = 0;

    if (!VirtualProtect(
        target, 6,
        PAGE_EXECUTE_READWRITE,
        &oldProtection))
        return false;

    memcpy(originalBytes, target, 6);

    const BYTE expectedBytes[6] = {
    0x8B, 0x53, 0x1C,
    0x8B, 0x43, 0x14
    };

    if (memcmp(originalBytes, expectedBytes, 6) != 0)
    {
        DWORD unused;
        VirtualProtect(target, 6, oldProtection, &unused);
        return false;
    }

    // Write JMP rel32 to InputHook.
    intptr_t relative =
        reinterpret_cast<BYTE*>(&InputHook) -
        (target + 5);

    BYTE patch[6] = { 0xE9 };

    int32_t displacement =
        static_cast<int32_t>(relative);

    memcpy(patch + 1, &displacement, 4);
    patch[5] = 0x90;

    memcpy(target, patch, sizeof(patch));

    FlushInstructionCache(
        GetCurrentProcess(), target, sizeof(patch));

    DWORD unused = 0;
    VirtualProtect(
        target, 6, oldProtection, &unused);

    hookInstalled = true;
    return true;
}

extern "C" __declspec(dllexport)
BOOL StartControllerHook()
{
    return InstallHook() ? TRUE : FALSE;
}
