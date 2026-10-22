// Exercise the actual private HID decoder and game adapter without starting HID I/O.
#include "../RetroInput/DualSense.cpp"
#include "../RetroInput/Games/TheSuffering/InputMapping.h"
#include "../RetroInput/Games/TheSuffering/ButtonRenderer.h"
#include "../RetroInput/Games/TheSuffering/Prompts.cpp"
#include <cassert>
#include <cmath>
std::atomic<int> g_controllerType{0};
std::atomic<bool> g_controllerActive{true};
static bool testGlyphs = true;
extern "C" BOOL AreInlineGlyphsEnabled() { return testGlyphs ? TRUE : FALSE; }
static std::wstring capturedText;
static wchar_t* __fastcall CaptureText(void*, void*, const wchar_t* text, int) {
    capturedText = text;
    return nullptr;
}
using namespace RetroInput::TheSuffering::ButtonRenderer;
static GlyphRecord metrics = {0, L'X', 2, 44, 3, 4, 16, 1};
static GlyphRecord* __fastcall FakeLookup(void*, void*, const wchar_t*) { return &metrics; }
static float expectedPixels = 24;
static int expectedX = 100, expectedY = 196;
static int __fastcall CheckDraw(void* texture, void*, int x, int y, int sourceX, int sourceY,
    int width, int height, const void* clip, unsigned flags, const std::uint32_t* colours,
    float sx, float sy) {
    assert(texture == reinterpret_cast<void*>(0x5678));
    assert(x == expectedX && y == expectedY && sourceX == 0 && sourceY == 0);
    assert(width == 128 && height == 128);
    assert(clip == reinterpret_cast<void*>(0x1234) && flags == 1);
    assert(colours[0] == 0x80FFFFFFu && colours[1] == 0x40FFFFFFu);
    assert(colours[2] == 0x20FFFFFFu && colours[3] == 0x00FFFFFFu);
    assert(std::abs(width * sx - expectedPixels) < 0.001f && sx == sy);
    return 77;
}
int main() {
    RetroInput::TheSuffering::InputMapping originalMapping;
    for (unsigned format = 0; format < 3; ++format) {
        for (unsigned clicks : {0u, 0x40u, 0x80u, 0xC0u}) {
            BYTE report[78] = {};
            report[0] = format == 2 ? 0x31 : 0x01;
            const unsigned base = format == 2 ? 2 : 1;
            for (unsigned i = 0; i < 4; ++i) report[base+i] = 128;
            report[format == 1 ? 5 : base+7] = 8;
            report[format == 1 ? 6 : base+8] = static_cast<BYTE>(clicks);
            XINPUT_GAMEPAD pad = {};
            assert(Decode(report, format == 0 ? 64 : 78, pad));
            assert(bool(pad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) == bool(clicks & 0x40));
            assert(bool(pad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) == bool(clicks & 0x80));
            float state[256] = {};
            state[0x1E] = 0.75f;
            originalMapping.Apply(0x005B5418, state, pad);
            assert(state[0x2F] == (clicks & 0x40 ? 1.0f : 0.0f));
            assert(state[0x30] == (clicks & 0x80 ? 1.0f : 0.0f));
            assert(state[0x1E] == 0.75f);
        }
    }
    XINPUT_GAMEPAD xbox = {};
    xbox.wButtons = XINPUT_GAMEPAD_LEFT_THUMB | XINPUT_GAMEPAD_RIGHT_THUMB;
    float state[256] = {};
    originalMapping.Apply(0x005B5418, state, xbox);
    assert(state[0x2F] == 1 && state[0x30] == 1);
    xbox.wButtons = 0;
    state[0x2F] = 0.75f;
    originalMapping.Apply(0x005B5418, state, xbox);
    assert(state[0x2F] == 0.75f);
    originalLookup = reinterpret_cast<Lookup>(&FakeLookup);
    const wchar_t code = 0xE000, ordinary = L'A';
    auto record = LookupHook(reinterpret_cast<void*>(0x1111), nullptr, &code);
    assert(record->width == 24 && record->right == 4 && (record->left & 0x3F) == 3);
    assert(LookupHook(nullptr, nullptr, &ordinary) == &metrics);
    metrics.width = 13;
    assert(LookupHook(reinterpret_cast<void*>(0x1111), nullptr, &code)->width == 20);
    metrics.width = 200;
    assert(LookupHook(reinterpret_cast<void*>(0x1111), nullptr, &code)->width == 255);
    const RetroInput::Buttons::ButtonImage image = {nullptr, 0, 0, 128, 128};
    const std::uint32_t colours[4] = {0x80AABBCC,0x40AABBCC,0x20AABBCC,0x00AABBCC};
    originalDraw = reinterpret_cast<Draw>(&CheckDraw);
    assert(DrawButtonImage(reinterpret_cast<void*>(0x5678), image, 100, 200, 24, 20,
        reinterpret_cast<void*>(0x1234), 0x401, colours, 1, 1) == 77);
    for (unsigned kind : {10u, 11u, 14u, 15u, 25u, 26u}) {
        const bool stick = kind >= 25;
        expectedPixels = stick ? 26.4f : 18.0f;
        expectedX = stick ? 99 : 103;
        expectedY = stick ? 195 : 199;
        assert(DrawButtonImage(reinterpret_cast<void*>(0x5678), image, 100, 200, 24, 20,
            reinterpret_cast<void*>(0x1234), 0x401, colours, 1, 1,
            GetPromptArtworkScale(kind)) == 77);
    }
    for (unsigned kind : {0u, 1u, 8u, 9u, 12u, 13u, 16u, 21u})
        assert(GetPromptArtworkScale(kind) == 1.0f);
    originalTextFunction = reinterpret_cast<TextFunction>(&CaptureText);
    for (int controller : {1, 2, 3, 4, 5, 6}) {
        g_controllerType.store(controller);
        const bool ps = RetroInput::Controllers::IsPlayStation(controller);
        for (bool glyphs : {false, true}) {
            testGlyphs = glyphs;
            ProcessControllerPrompt(nullptr, nullptr, L"ESC - Cancel / ESC / ESC", 0);
            const std::wstring menu = glyphs ? (ps ? L"\xE015" : L"\xE017") :
                (ps && controller != 6 ? L"Options" : L"Start");
            assert(capturedText == menu + L" - Cancel / " + menu + L" / " + menu);
            ProcessControllerPrompt(nullptr, nullptr, L"<Map>", 0);
            const std::wstring map = glyphs ? (!ps ? L"\xE018" : L"\xE016") :
                (!ps ? L"View" : (controller == 3 ? L"Share" : (controller == 6 ? L"Select" : L"Create")));
            assert(capturedText == map);
        }
    }
    g_controllerActive.store(false);
    ProcessControllerPrompt(nullptr, nullptr, L"ESC - Cancel / ESC", 0);
    assert(capturedText == L"ESC - Cancel / ESC");
    return 0;
}

