#define RETROINPUT_TTB
#include "../RetroInput/Games/TheSufferingTTB/InputMapping.h"
#include "../RetroInput/Core/XInputAggregator.h"
#include "../RetroInput/Games/TheSuffering/InputMapping.h"
#include <array>
#include <cassert>

using RetroInput::TheSufferingTTB::InputMapping;
using namespace RetroInput::TheSufferingTTB;

int main()
{
    RetroInput::TheSuffering::InputMapping originalMapping;
    InputMapping mapping;
    std::array<float, 256> keyboard = {};
    std::array<float, 14> mouse = {};
    XINPUT_GAMEPAD pad = {};
    const struct { WORD button; unsigned key; } bindings[] = {
        {XINPUT_GAMEPAD_LEFT_THUMB, 0x1D}, {XINPUT_GAMEPAD_RIGHT_THUMB, 0x21},
        {XINPUT_GAMEPAD_X, 0x1C}, {XINPUT_GAMEPAD_A, 0x39},
        {XINPUT_GAMEPAD_LEFT_SHOULDER, 0x2E}, {XINPUT_GAMEPAD_RIGHT_SHOULDER, 0x2D},
        {XINPUT_GAMEPAD_START, 0x01}, {XINPUT_GAMEPAD_BACK, 0x03},
        {XINPUT_GAMEPAD_DPAD_UP, 0x02}, {XINPUT_GAMEPAD_DPAD_DOWN, 0x13},
        {XINPUT_GAMEPAD_DPAD_LEFT, 0x4B}, {XINPUT_GAMEPAD_DPAD_RIGHT, 0x4D}
    };
    for (const auto binding : bindings) {
        pad = {}; pad.wButtons = binding.button; keyboard.fill(0);
        mapping.Apply(KeyboardRecord, keyboard.data(), pad);
        for (unsigned i = 0; i < keyboard.size(); ++i)
            assert(keyboard[i] == (i == binding.key ? 1.0f : 0.0f));
    }
    // Tap North cycles Q on release; a 300ms hold sends Z without Q.
    mapping.Reset(); pad={}; pad.wButtons=XINPUT_GAMEPAD_Y; keyboard.fill(0);
    mapping.Apply(KeyboardRecord,keyboard.data(),pad,100);
    assert(!keyboard[0x10] && !keyboard[0x2C]);
    keyboard.fill(0);pad={};mapping.Apply(KeyboardRecord,keyboard.data(),pad,399);
    assert(keyboard[0x10] && !keyboard[0x2C]);
    keyboard.fill(0);mapping.Apply(KeyboardRecord,keyboard.data(),pad,400);assert(!keyboard[0x10]);
    pad.wButtons=XINPUT_GAMEPAD_Y;mapping.Apply(KeyboardRecord,keyboard.data(),pad,500);
    keyboard.fill(0);mapping.Apply(KeyboardRecord,keyboard.data(),pad,799);assert(!keyboard[0x2C] && !keyboard[0x10]);
    keyboard.fill(0);mapping.Apply(KeyboardRecord,keyboard.data(),pad,800);assert(keyboard[0x2C] && !keyboard[0x10]);
    keyboard.fill(0);mapping.Apply(KeyboardRecord,keyboard.data(),pad,1100);assert(keyboard[0x2C] && !keyboard[0x10]);
    pad={};keyboard.fill(0);mapping.Apply(KeyboardRecord,keyboard.data(),pad,1200);assert(!keyboard[0x10] && !keyboard[0x2C]);
    pad.wButtons=XINPUT_GAMEPAD_Y;mapping.Apply(KeyboardRecord,keyboard.data(),pad,1300);mapping.Reset();
    pad={};keyboard.fill(0);mapping.Apply(KeyboardRecord,keyboard.data(),pad,1310);assert(!keyboard[0x10] && !keyboard[0x2C]);
    pad.wButtons=XINPUT_GAMEPAD_Y;mapping.Apply(KeyboardRecord,keyboard.data(),pad,0xFFFFFFF0u);
    keyboard.fill(0);mapping.Apply(KeyboardRecord,keyboard.data(),pad,0x11Cu);assert(keyboard[0x2C] && !keyboard[0x10]);
    mapping.Reset();keyboard.fill(0);

    pad.wButtons = XINPUT_GAMEPAD_B;
    keyboard.fill(0); mapping.Apply(KeyboardRecord, keyboard.data(), pad);
    for (float value : keyboard) assert(value == 0);

    // Each sample begins with the game's fresh physical keyboard state.
    DWORD now = 1000;
    auto sample = [&](SHORT x, SHORT y) {
        keyboard.fill(0); pad = {}; pad.sThumbLX = x; pad.sThumbLY = y;
        mapping.Apply(KeyboardRecord, keyboard.data(), pad, now);
    };
    sample(0, 0); assert(keyboard[0x2A] == 0);
    sample(4915, 0); assert(keyboard[0xCD] == 0 && keyboard[0x2A] == 0); // <=15%
    sample(5000, 0); assert(keyboard[0xCD] == 1 && keyboard[0x2A] == 1);
    sample(15000, 0); assert(keyboard[0x2A] == 1); // hold walk inside buffer
    sample(16000, 0); assert(keyboard[0x2A] == 0); // >48%
    sample(14500, 0); assert(keyboard[0x2A] == 0); // hold run inside buffer
    sample(13600, 0); assert(keyboard[0x2A] == 1); // <42%
    sample(0, 0); assert(keyboard[0x2A] == 1 && keyboard[0xCD] == 0);
    now += 249; sample(1000, 0); assert(keyboard[0x2A] == 1 && keyboard[0xCD] == 0);
    ++now; sample(0, 0); assert(keyboard[0x2A] == 0);
    sample(14500, 0); assert(keyboard[0x2A] == 1); // new movement starts walking
    mapping.Reset(); sample(32767, 0); assert(keyboard[0x2A] == 0); // immediate full tilt runs
    sample(4000, 4000); assert(keyboard[0xC8] == 1 && keyboard[0xCD] == 1 && keyboard[0x2A] == 1);
    sample(-32768, -32768); assert(keyboard[0xCB] == 1 && keyboard[0xD0] == 1 && keyboard[0x2A] == 0);
    sample(200, 7000); assert(keyboard[0xC8] == 1 && keyboard[0xCD] == 0);
    keyboard.fill(0); keyboard[0x2A] = 1; pad = {}; pad.sThumbLX = 32767;
    mapping.Apply(KeyboardRecord, keyboard.data(), pad);
    assert(keyboard[0x2A] == 1); // never erase physical Shift while running
    pad = {}; keyboard.fill(0); mapping.Apply(KeyboardRecord, keyboard.data(), pad);
    assert(keyboard[0x2A] == 0);

    pad = {}; pad.bRightTrigger = 255; pad.bLeftTrigger = 255; pad.wButtons = XINPUT_GAMEPAD_B;
    mapping.Apply(MouseRecord, mouse.data(), pad);
    assert(mouse[6] == 1 && mouse[7] == 1 && mouse[8] == 1);
    mouse.fill(0); pad = {}; pad.bRightTrigger = 30; pad.bLeftTrigger = 30;
    mapping.Apply(MouseRecord, mouse.data(), pad);
    assert(mouse[6] == 0 && mouse[7] == 0);
    mouse.fill(0); pad = {}; pad.sThumbRX = -32768; pad.sThumbRY = 32767;
    mouse[0] = 0.1f; mapping.Apply(MouseRecord, mouse.data(), pad);
    assert(std::abs(mouse[0] - 0.85f) < 0.0001f && mouse[2] == 0.75f);
    mouse.fill(0); pad.sThumbRX = 32767; pad.sThumbRY = -32768;
    mapping.Apply(MouseRecord, mouse.data(), pad);
    assert(mouse[1] == 0.75f && mouse[3] == 0.75f);
    mapping.Apply(MouseRecord, nullptr, pad);

    // Stop-walk hold: no rearming from drift, immediate cancellation for run/reset,
    // no hold when stopping a run, and unsigned clock rollover.
    mapping.Reset(); now = 2000;
    sample(6000, 0); sample(0, 0); assert(keyboard[0x2A] == 1);
    now += 50; sample(32767, 0); assert(keyboard[0x2A] == 0);
    sample(0, 0); assert(keyboard[0x2A] == 0);
    sample(6000, 0); sample(0, 0); assert(keyboard[0x2A] == 1);
    mapping.Reset(); sample(0, 0); assert(keyboard[0x2A] == 0);
    now = 0xFFFFFFF0u; sample(6000, 0); sample(0, 0);
    now += 249; sample(1000, 1000); assert(keyboard[0x2A] == 1);
    ++now; sample(1000, 1000); assert(keyboard[0x2A] == 0);
    now += 100; sample(1000, 1000); assert(keyboard[0x2A] == 0);
    // A fresh idle pad and unrelated buttons never synthesize Shift.
    mapping.Reset(); sample(0, 0); assert(keyboard[0x2A] == 0);

    // Gentle input must survive XInput aggregation and activity selection.
    RetroController::XInputAggregator aggregator;
    std::array<XINPUT_STATE, XUSER_MAX_COUNT> states = {};
    std::array<bool, XUSER_MAX_COUNT> connected = {true, true, false, false};
    states[0].Gamepad.sThumbLY = 5000;
    auto result = aggregator.Update(states, connected);
    assert(result.gamepad.sThumbLY == 5000 && RetroInput::Controllers::HasActivity(result.gamepad));
    states[1].Gamepad.sThumbLX = states[1].Gamepad.sThumbLY = 4000;
    result = aggregator.Update(states, connected);
    assert(result.gamepad.sThumbLX == 4000 && result.gamepad.sThumbLY == 4000);
    connected[1] = false;
    result = aggregator.Update(states, connected);
    assert(result.gamepad.sThumbLX == 0 && result.gamepad.sThumbLY == 5000);
    states[0].Gamepad.sThumbLY = 1000;
    result = aggregator.Update(states, connected);
    assert(!RetroInput::Controllers::HasActivity(result.gamepad));

    // The original adapter still maps its own stick clicks to V/B, not Ctrl/F.
    keyboard.fill(0); pad = {}; pad.wButtons = XINPUT_GAMEPAD_LEFT_THUMB | XINPUT_GAMEPAD_RIGHT_THUMB;
    originalMapping.Apply(0x005B5418, keyboard.data(), pad);
    assert(keyboard[0x2F] == 1 && keyboard[0x30] == 1 && keyboard[0x1D] == 0 && keyboard[0x21] == 0);
}

