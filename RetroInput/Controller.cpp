#include "pch.h"
#include <Windows.h>
#include <Xinput.h>
#include <atomic>
#include "ControllerSelector.h"
#if defined(RETROINPUT_TTB)
#include "Games/TheSufferingTTB/InputMapping.h"
static RetroInput::TheSufferingTTB::InputMapping g_ttbMapping;
static int g_ttbOwner = 0;
#else
#include "Games/TheSuffering/InputMapping.h"
static RetroInput::TheSuffering::InputMapping g_sufferingMapping;
static int g_sufferingOwner = 0;
#endif
#pragma comment(lib, "Xinput9_1_0.lib")

std::atomic<bool> g_controllerActive{ false };
std::atomic<int> g_controllerType{ 0 }; // 0=keyboard, 1=Xbox, 2=DualSense, 3=DualShock4, 4=SHIELD, 5=Backbone, 6=VX2, 7=Joy-Con pair
static RetroController::ControllerSelector g_selector;

// Called by the game input hook.

void __stdcall InjectControllerInput(DWORD deviceRecord, float* state)
{
    if (state == nullptr)
        return;

#if defined(RETROINPUT_TTB)
    if (*reinterpret_cast<const DWORD*>(deviceRecord) != 2) {
        g_ttbMapping.Reset();
        g_ttbOwner = 0;
        return;
    }
    constexpr DWORD keyboardRecord = RetroInput::TheSufferingTTB::KeyboardRecord;
#else
    constexpr DWORD keyboardRecord = 0x005B5418;
#endif

    bool physicalKeyboard = false;
    // Detect physical keyboard activity before controller input is injected.
    if (deviceRecord == keyboardRecord)
    {
        for (int scanCode = 0; scanCode < 256; ++scanCode)
        {
            if (state[scanCode] > 0.5f)
            {
                g_controllerActive.store(false);
                g_controllerType.store(0);
                physicalKeyboard = true;
                break;
            }
        }
    }

    const RetroController::Selection selection = g_selector.Poll(physicalKeyboard);
#if defined(RETROINPUT_TTB)
    if (physicalKeyboard || selection.type != g_ttbOwner || selection.type == 0)
        g_ttbMapping.Reset();
    g_ttbOwner = selection.type;
#else
    if (physicalKeyboard || selection.type != g_sufferingOwner || selection.type == 0)
        g_sufferingMapping.Reset();
    g_sufferingOwner = selection.type;
#endif
    if (physicalKeyboard)
        return;

    g_controllerType.store(selection.type);
    g_controllerActive.store(selection.type != 0);
    if (selection.type == 0)
        return;

    const XINPUT_GAMEPAD& pad = selection.gamepad;

#if defined(RETROINPUT_TTB)
    g_ttbMapping.Apply(deviceRecord, state, pad);
#else
    g_sufferingMapping.Apply(deviceRecord, state, pad);
#endif
}
