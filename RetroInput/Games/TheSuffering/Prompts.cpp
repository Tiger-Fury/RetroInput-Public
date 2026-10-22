
#include "pch.h"
#include <Windows.h>
#include <atomic>
#include <string>
#include <cstdint>
#include <cstring>
#include "../../Core/ControllerTypes.h"

extern std::atomic<bool> g_controllerActive;
extern std::atomic<int> g_controllerType;
extern "C" BOOL AreInlineGlyphsEnabled();

namespace
{
    constexpr uintptr_t TEXT_HOOK_ADDRESS = 0x0040DE80;
    constexpr uintptr_t TEXT_RETURN_ADDRESS = 0x0040DE87;

    BYTE originalTextBytes[7] = {};
    bool textHookInstalled = false;

    using TextFunction = wchar_t* (__thiscall*)(
        void* object, const wchar_t* text, int context);

    TextFunction originalTextFunction = nullptr;

    // Replace every occurrence of a string.
    void ReplaceAll(
        std::wstring& text,
        const wchar_t* find,
        const wchar_t* replacement)
    {
        const std::wstring needle(find);
        const std::wstring value(replacement);

        size_t position = 0;

        while ((position = text.find(needle, position)) !=
            std::wstring::npos)
        {
            text.replace(position, needle.length(), value);
            position += value.length();
        }
    }

    wchar_t* __fastcall ProcessControllerPrompt(
        void* object,
        void* /*unusedEDX*/,
        const wchar_t* text,
        int context)
    {
        if (!originalTextFunction || !text)
            return nullptr;

        // Preserve the game's normal keyboard prompts.
        if (!g_controllerActive.load())
            return originalTextFunction(object, text, context);

        std::wstring modified(text);

        const int controllerType = g_controllerType.load();
        const bool ps = RetroInput::Controllers::IsPlayStation(controllerType);
        const bool ps4 = controllerType == 3;
        const bool vx2 = controllerType == 6;
        const bool nintendo = controllerType == 7;
        const bool glyphs = AreInlineGlyphsEnabled() != FALSE;
        const wchar_t* cross = glyphs ? L"\xE000" : L"Cross";
        const wchar_t* square = glyphs ? L"\xE001" : L"Square";
        const wchar_t* circle = glyphs ? L"\xE002" : L"Circle";
        const wchar_t* triangle = glyphs ? L"\xE003" : L"Triangle";
        const wchar_t* xboxA = glyphs ? L"\xE004" : (nintendo ? L"B" : L"A");
        const wchar_t* xboxB = glyphs ? L"\xE005" : (nintendo ? L"A" : L"B");
        const wchar_t* xboxX = glyphs ? L"\xE006" : (nintendo ? L"Y" : L"X");
        const wchar_t* xboxY = glyphs ? L"\xE007" : (nintendo ? L"X" : L"Y");
        const wchar_t* leftShoulder = glyphs ? (ps ? L"\xE008" : L"\xE00C") : (ps ? L"L1" : nintendo ? L"L" : L"LB");
        const wchar_t* rightShoulder = glyphs ? (ps ? L"\xE009" : L"\xE00D") : (ps ? L"R1" : nintendo ? L"R" : L"RB");
        const wchar_t* leftTrigger = glyphs ? (ps ? L"\xE00A" : L"\xE00E") : (ps ? L"L2" : nintendo ? L"ZL" : L"LT");
        const wchar_t* rightTrigger = glyphs ? (ps ? L"\xE00B" : L"\xE00F") : (ps ? L"R2" : nintendo ? L"ZR" : L"RT");
        const wchar_t* leftStick = glyphs ? L"\xE019" : L"Left Stick";
        // Match the existing controller layout, changing only the displayed names.
        ReplaceAll(modified, L"<Reload>", ps ? circle : xboxB);
        ReplaceAll(modified, L"<Attack>", rightTrigger);
        ReplaceAll(modified, L"<Throw>", leftTrigger);
        ReplaceAll(modified, L"<Jump>", ps ? cross : xboxA);
        ReplaceAll(modified, L"<FirstPerson>", leftShoulder);
        ReplaceAll(modified, L"<Use>", ps ? square : xboxX);
        ReplaceAll(modified, L"<UseHealth>", rightShoulder);
        ReplaceAll(modified, L"<Health>", rightShoulder);
        ReplaceAll(modified, L"<Flashlight>", glyphs ? L"\xE011" : L"D-pad Down");
        ReplaceAll(modified, L"Use F to turn flashlight on and off", glyphs ? L"Use \xE011 to turn flashlight on and off" : L"Use D-pad Down to turn flashlight on and off");
        ReplaceAll(modified, L"Use <Inventory> to navigate inventory", glyphs ? L"Use \xE014 to navigate inventory" : L"Use D-pad Left/Right to navigate inventory");
        ReplaceAll(modified, L"<Inventory>", glyphs ? L"\xE010" : L"D-pad Up");
        ReplaceAll(modified, L"<LadderMove>", leftStick);
        ReplaceAll(modified, L"<Insanity>", ps ? triangle : xboxY);
        ReplaceAll(modified, L"<Map>", glyphs ? (ps ? L"\xE016" : L"\xE018") : (ps ? (ps4 ? L"Share" : (vx2 ? L"Select" : L"Create")) : nintendo ? L"Minus" : L"View"));
        ReplaceAll(modified, L"ESC - Cancel", glyphs ? (ps ? L"\xE015 - Cancel" : L"\xE017 - Cancel") : (ps && !vx2 ? L"Options - Cancel" : nintendo ? L"Plus - Cancel" : L"Start - Cancel"));
        ReplaceAll(modified, L"ESC", glyphs ? (ps ? L"\xE015" : L"\xE017") : (ps && !vx2 ? L"Options" : nintendo ? L"Plus" : L"Start"));
        ReplaceAll(modified, L"Return - Save Your Progress", ps ? (glyphs ? L"\xE001 - Save Your Progress" : L"Square - Save Your Progress") : (glyphs ? L"\xE006 - Save Your Progress" : L"X - Save Your Progress"));
        ReplaceAll(modified, L"Return - Load Selected Saved Game", ps ? (glyphs ? L"\xE001 - Load Selected Saved Game" : L"Square - Load Selected Saved Game") : (glyphs ? L"\xE006 - Load Selected Saved Game" : L"X - Load Selected Saved Game"));
        ReplaceAll(modified, L"Use W, A, S, D to move", glyphs ? L"Use \xE019 to move" : L"Use Left Stick to move");
        ReplaceAll(modified, L"Use the Mouse to look and turn", glyphs ? L"Use the \xE01A to look and turn" : L"Use the Right Stick to look and turn");

        return originalTextFunction(
            object, modified.c_str(), context);
    }

    BYTE* CreateTextTrampoline()
    {
        BYTE* trampoline = static_cast<BYTE*>(
            VirtualAlloc(
                nullptr,
                12,
                MEM_COMMIT | MEM_RESERVE,
                PAGE_EXECUTE_READWRITE));

        if (!trampoline)
            return nullptr;

        memcpy(trampoline, originalTextBytes, 7);

        // Jump back after the overwritten instructions.
        trampoline[7] = 0xE9;

        const intptr_t relative =
            TEXT_RETURN_ADDRESS -
            reinterpret_cast<uintptr_t>(trampoline + 12);

        const int32_t displacement =
            static_cast<int32_t>(relative);

        memcpy(trampoline + 8, &displacement, 4);

        FlushInstructionCache(
            GetCurrentProcess(), trampoline, 12);

        return trampoline;
    }
}

// Bridge the game's __thiscall to our __fastcall handler.
__declspec(naked) void TextHook()
{
    __asm
    {
        xor edx, edx
        jmp ProcessControllerPrompt
    }
}

extern "C" BOOL InstallTextHook()
{
    if (textHookInstalled)
        return TRUE;

    BYTE* target =
        reinterpret_cast<BYTE*>(TEXT_HOOK_ADDRESS);

    const BYTE expected[7] = {
        0x6A, 0xFF,
        0x68, 0x3B, 0xE0, 0x57, 0x00
    };

    if (memcmp(target, expected, 7) != 0)
        return FALSE;

    memcpy(originalTextBytes, target, 7);

    BYTE* trampoline = CreateTextTrampoline();

    if (!trampoline)
        return FALSE;

    originalTextFunction =
        reinterpret_cast<TextFunction>(trampoline);

    DWORD oldProtection = 0;

    if (!VirtualProtect(
        target,
        7,
        PAGE_EXECUTE_READWRITE,
        &oldProtection))
    {
        VirtualFree(trampoline, 0, MEM_RELEASE);
        originalTextFunction = nullptr;
        return FALSE;
    }

    BYTE patch[7] = { 0xE9 };

    const intptr_t relative =
        reinterpret_cast<uintptr_t>(&TextHook) -
        reinterpret_cast<uintptr_t>(target + 5);

    const int32_t displacement =
        static_cast<int32_t>(relative);

    memcpy(patch + 1, &displacement, 4);

    patch[5] = 0x90;
    patch[6] = 0x90;

    memcpy(target, patch, 7);

    FlushInstructionCache(
        GetCurrentProcess(), target, 7);

    DWORD unused = 0;

    VirtualProtect(
        target,
        7,
        oldProtection,
        &unused);

    textHookInstalled = true;
    return TRUE;
}

