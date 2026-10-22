#include "pch.h"
#include <Windows.h>

extern "C" BOOL StartControllerHook();
#if !defined(RETROINPUT_TTB)
extern "C" BOOL InstallTextHook();
#endif
#include "DualSense.h"
#include "DualShock4.h"
#include "Shield.h"
#include "Backbone.h"
#include "Vx2.h"
#include "JoyCon.h"
#if !defined(RETROINPUT_TTB)
#include "Games/TheSuffering/ButtonRenderer.h"
#else
#include "Games/TheSufferingTTB/ButtonRenderer.h"
#include "Games/TheSufferingTTB/Prompts.h"
#endif

extern "C" BOOL AreInlineGlyphsEnabled()
{
#if defined(RETROINPUT_TTB)
    return RetroInput::TheSufferingTTB::ButtonRenderer::InlineEnabled() ? TRUE : FALSE;
#else
    return RetroInput::TheSuffering::ButtonRenderer::InlineEnabled() ? TRUE : FALSE;
#endif
}

DWORD WINAPI ControllerThread(LPVOID)
{
#if defined(RETROINPUT_TTB)
    RetroInput::TheSufferingTTB::RenderingSupport::Log("TTB DLL startup worker entered");
#else
    HMODULE imagesModule = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&ControllerThread), &imagesModule);
    const bool imagesReady = RetroInput::Buttons::InitializeButtonImages(imagesModule);
#if defined(_M_IX86)
    RetroInput::TheSuffering::ButtonRenderer::Log(imagesReady ?
        "Decoded DualSense, DualShock4, Xbox Series and Nintendo image packs" : "An image pack is unavailable; using per-pack fallback");
#endif
#endif
    StartDualSenseReader();
    StartDualShock4Reader();
    StartShieldReader();
    StartBackboneReader();
    StartVx2Reader();
    StartJoyConReader();
#if defined(RETROINPUT_TTB)
    const BOOL inputReady = StartControllerHook();
    RetroInput::TheSufferingTTB::RenderingSupport::Log("Controller hook installed", inputReady);
    if (inputReady)
    {
        HMODULE imagesModule = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&ControllerThread), &imagesModule);
        const bool ready = RetroInput::Buttons::InitializeButtonImages(imagesModule);
        RetroInput::TheSufferingTTB::RenderingSupport::Log(ready ?
            "Decoded image packs" : "An image pack is unavailable; using fallback");
        const bool rendered = RetroInput::TheSufferingTTB::ButtonRenderer::Install();
        RetroInput::TheSufferingTTB::RenderingSupport::Log("Prompt renderer setup", rendered);
        const bool prompts = RetroInput::TheSufferingTTB::Prompts::Install();
        RetroInput::TheSufferingTTB::RenderingSupport::Log("Prompt expansion setup", prompts);
    }
#else
    if (StartControllerHook())
    {
        InstallTextHook();
        RetroInput::TheSuffering::ButtonRenderer::Install();
    }
#endif

    return 0;
}

BOOL APIENTRY DllMain(
    HMODULE hModule,
    DWORD reason,
    LPVOID reserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);

        HANDLE thread = CreateThread(
            nullptr,
            0,
            ControllerThread,
            nullptr,
            0,
            nullptr
        );

        if (thread)
            CloseHandle(thread);
    }

    return TRUE;
}
