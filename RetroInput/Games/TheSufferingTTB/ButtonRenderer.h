#pragma once
#include <Windows.h>
#include <intrin.h>
#include <cstdint>
#include <cstring>
#include <atomic>
#include <cmath>
#include "../../Core/ButtonImages.h"
#include "../../Core/ControllerTypes.h"
#include "RenderingSupport.h"

extern std::atomic<int> g_controllerType;

// Native controller glyph rendering for the verified SufferingTTB.exe build.
// Included only by dllmain.cpp. No shared controller policy belongs here.
namespace RetroInput::TheSufferingTTB::ButtonRenderer {
#if defined(_M_IX86)
inline constexpr float ButtonDisplayScale = 1.5f;
using Draw = int (__thiscall*)(void*, int, int, int, int, int, int,
    const void*, unsigned, const std::uint32_t*, float, float, int);
using Create = void* (__thiscall*)(void*, unsigned, unsigned, int, unsigned, void*);
using Release = int (__thiscall*)(void*, int);
using LockedRect = RenderingSupport::LockedRect;
using Lock = HRESULT (__stdcall*)(void*, unsigned, LockedRect*, const RECT*, DWORD);
using Unlock = HRESULT (__stdcall*)(void*, unsigned);

static Draw originalDraw = nullptr;
struct AtlasTexture {
    const Buttons::AtlasImage* source = nullptr;
    void* texture = nullptr;
    bool failed = false;
};
static AtlasTexture atlasTextures[5]; // Legacy plus the four platform packs, independently owned.
static DWORD renderThread = 0;

inline void Log(const char* message, long value = 0) { RenderingSupport::Log(message, value); }

template<class T> inline T Method(void* object, unsigned offset) {
    return reinterpret_cast<T>((*reinterpret_cast<std::uintptr_t**>(object))[offset / 4]);
}
inline void ReleaseAtlas(AtlasTexture& atlas) {
    if (!atlas.texture) return;
    void* old = atlas.texture;
    atlas.texture = nullptr;
    Method<Release>(old, 0x30)(old, 0);
    Log("Released atlas owner reference");
}
inline bool CreateAtlas(const Buttons::AtlasImage& image, AtlasTexture& atlas) {
    // 32-bit, alpha selector 0x80000, drawable flag 1; no render-target flag.
    atlas.texture = reinterpret_cast<Create>(0x0044E460)(
        reinterpret_cast<void*>(0x0063C2E0), image.width, image.height,
        32, 0x00080001, nullptr);
    if (!atlas.texture) { Log("Engine texture creation failed"); return false; }
    void* gpu = *reinterpret_cast<void**>(static_cast<unsigned char*>(atlas.texture) + 0x68);
    if (!gpu) { Log("Created resource has no GPU texture"); ReleaseAtlas(atlas); return false; }
    LockedRect rect = {};
    // Verified engine interface offsets (do not substitute IDirect3DTexture9 layout).
    const HRESULT locked = Method<Lock>(gpu, 0x4C)(gpu, 0, &rect, nullptr, 0);
    if (FAILED(locked)) { Log("Texture lock failed", locked); ReleaseAtlas(atlas); return false; }
    const bool valid = RenderingSupport::CopyPixels(image, rect);
    const HRESULT unlocked = Method<Unlock>(gpu, 0x50)(gpu, 0);
    if (!valid || FAILED(unlocked)) {
        Log("Invalid pitch/pixels or unlock failure", rect.pitch);
        ReleaseAtlas(atlas); return false;
    }
    Log("Uploaded controller image pack; width", image.width);
    Log("Controller image pack height", image.height);
    Log("Controller image pack actual pitch", rect.pitch);
    return true;
}
inline void* GetNativeAtlas(const Buttons::AtlasImage& image) {
    for (auto& atlas : atlasTextures) {
        if (atlas.source && atlas.source != &image) continue;
        atlas.source = &image;
        if (!atlas.texture && !atlas.failed && !CreateAtlas(image, atlas)) {
            atlas.failed = true;
            Log("Inline atlas creation failed; using X fallback");
        }
        return atlas.texture;
    }
    return nullptr;
}

// Private-use character order belongs to this game's prompt transport only.
// The shared service receives logical controls, never PUA codes or atlas indices.
inline Buttons::ButtonImage GetPromptButtonImage(unsigned kind,
    Buttons::Controller activeController = Buttons::Controller::Xbox) {
    if (kind >= static_cast<unsigned>(Buttons::Button::Count)) return {};
    return Buttons::GetButtonImage(activeController, static_cast<Buttons::Button>(kind));
}

// Translate shared artwork into this game's native drawing.
inline float GetPromptArtworkScale(unsigned kind) {
    const auto button = static_cast<Buttons::Button>(kind);
    if (button == Buttons::Button::LeftStick || button == Buttons::Button::RightStick) return 1.1f;
    if (button == Buttons::Button::LeftTrigger || button == Buttons::Button::RightTrigger) return 0.75f;
    return 1.0f;
}
inline int DrawButtonImage(void* nativeAtlas, const Buttons::ButtonImage& image,
    int x, int y, int width, int height, const void* clip, unsigned flags,
    const std::uint32_t* colours, float sx, float sy, int renderMode, float artworkScale = 1.0f) {
    std::uint32_t alphaColours[4];
    for (unsigned i = 0; i < 4; ++i)
        alphaColours[i] = (colours ? colours[i] & 0xFF000000u : 0xFF000000u) | 0xFFFFFFu;
    const float boxWidth = sx * static_cast<float>(width);
    const float lineHeight = sy * static_cast<float>(height);
    // Synthetic width already reserves the enlarged horizontal text advance.
    const float boxHeight = lineHeight * ButtonDisplayScale;
    const float scaleX = boxWidth / image.width, scaleY = boxHeight / image.height;
    const float scale = (scaleX < scaleY ? scaleX : scaleY) * artworkScale;
    const int glyphX = x + static_cast<int>(std::lround((boxWidth - image.width * scale) * 0.5f));
    const int glyphY = y - static_cast<int>(std::lround(2.0f * sy)) +
        static_cast<int>(std::lround((lineHeight - image.height * scale) * 0.5f));
    return originalDraw(nativeAtlas, glyphX, glyphY, image.x, image.y, image.width, image.height,
        clip, flags & ~0x403u, alphaColours, scale, scale, renderMode);
}

#pragma pack(push, 1)
struct GlyphRecord {
    std::uint32_t next;
    std::uint16_t character, rectangle, source;
    std::uint8_t left, right, width, page;
};
#pragma pack(pop)
static_assert(sizeof(GlyphRecord) == 14);
using Lookup = GlyphRecord* (__thiscall*)(void*, const wchar_t*);
static Lookup originalLookup = nullptr;
static volatile LONG inlineReady = 0;
constexpr unsigned MarkerBase = 0x7E00;
constexpr unsigned SlotCount = 256;
static_assert(MarkerBase + SlotCount <= 0x8000);
struct GlyphSlot {
    void* font;
    unsigned kind;
    GlyphRecord original, synthetic;
};
// Stable records for a render/measurement thread; no font arrays are modified.
// Existing entries are refreshed on every lookup, including reused font addresses.
static thread_local GlyphSlot glyphSlots[SlotCount] = {};
inline bool InlineEnabled() {
    return InterlockedCompareExchange(&inlineReady, 0, 0) != 0;
}
static GlyphRecord* __fastcall LookupHook(void* font, void*, const wchar_t* character) {
    if (!character || (*character < 0xE100 || *character >= 0xE100 + static_cast<unsigned>(Buttons::Button::Count)))
        return originalLookup(font, character);
    const unsigned kind = static_cast<unsigned>(*character - 0xE100);
    const wchar_t fallback = L'X';
    GlyphRecord* normal = originalLookup(font, &fallback);
    if (!normal || !normal->width) return normal;
    unsigned index = SlotCount;
    for (unsigned i = 0; i < SlotCount; ++i)
        if (glyphSlots[i].font == font && glyphSlots[i].kind == kind) { index = i; break; }
    if (index == SlotCount)
        for (unsigned i = 0; i < SlotCount; ++i)
            if (!glyphSlots[i].font) { index = i; break; }
    if (index == SlotCount) return normal; // Visible X if the bounded cache is full.
    auto& slot = glyphSlots[index];
    slot.font = font;
    slot.kind = kind;
    slot.original = *normal;
    slot.synthetic = *normal;
    slot.synthetic.next = 0;
    slot.synthetic.character = static_cast<std::uint16_t>(*character);
    slot.synthetic.source = static_cast<std::uint16_t>(MarkerBase + index);
    // Width measurement and batching consume the same record, keeping wrapping
    // and neighbouring words consistent with the larger image.
    const unsigned enlargedWidth = (static_cast<unsigned>(normal->width) * 3 + 1) / 2;
    slot.synthetic.width = static_cast<std::uint8_t>(enlargedWidth > 255 ? 255 : enlargedWidth);
    // Preserve lower-six-bit bearing; request white RGB with original text alpha.
    slot.synthetic.left |= 0x80;
    return &slot.synthetic;
}
inline bool InstallLookup() {
    const unsigned char expected[7] = { 0x8B, 0x44, 0x24, 0x04, 0x0F, 0xB7, 0x10 };
    auto target = reinterpret_cast<unsigned char*>(0x0048F820);
    if (memcmp(target, expected, sizeof(expected))) {
        Log("Inline glyph lookup signature rejected"); return false;
    }
    auto trampoline = static_cast<unsigned char*>(
        VirtualAlloc(nullptr, 12, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
    if (!trampoline) return false;
    memcpy(trampoline, expected, 7);
    trampoline[7] = 0xE9;
    const auto back = static_cast<std::int32_t>(
        reinterpret_cast<std::intptr_t>(target + 7) -
        reinterpret_cast<std::intptr_t>(trampoline + 12));
    memcpy(trampoline + 8, &back, 4);
    FlushInstructionCache(GetCurrentProcess(), trampoline, 12);
    originalLookup = reinterpret_cast<Lookup>(trampoline);
    DWORD protection = 0;
    if (!VirtualProtect(target, 7, PAGE_EXECUTE_READWRITE, &protection)) {
        originalLookup = nullptr; VirtualFree(trampoline, 0, MEM_RELEASE); return false;
    }
    unsigned char patch[7] = { 0xE9, 0, 0, 0, 0, 0x90, 0x90 };
    const auto displacement = static_cast<std::int32_t>(
        reinterpret_cast<std::intptr_t>(&LookupHook) -
        reinterpret_cast<std::intptr_t>(target + 5));
    memcpy(patch + 1, &displacement, 4);
    memcpy(target, patch, 7);
    FlushInstructionCache(GetCurrentProcess(), target, 7);
    DWORD ignored = 0;
    VirtualProtect(target, 7, protection, &ignored);
    InterlockedExchange(&inlineReady, 1);
    Log("Installed default controller glyph rendering: face/shoulder/trigger/D-pad/menu/stick");
    return true;
}

inline int RenderFontGlyph(void* texture,
    int x, int y, int sourceA, int sourceB, int width, int height,
    const void* clip, unsigned flags, const std::uint32_t* colours, float sx, float sy, int renderMode) {
    const DWORD thread = GetCurrentThreadId();
    if (!renderThread) renderThread = thread;
    if (renderThread != thread)
        return originalDraw(texture, x, y, sourceA, sourceB, width, height,
            clip, flags, colours, sx, sy, renderMode);
    // Synthetic source coordinates identify only our private-use glyph records.
    if (InlineEnabled() && sourceA >= static_cast<int>(MarkerBase) &&
        sourceA < static_cast<int>(MarkerBase + SlotCount)) {
        const auto& slot = glyphSlots[sourceA - MarkerBase];
        if (slot.font) {
            const int deviceType = g_controllerType.load();
            const auto controller = deviceType == 7 ? Buttons::Controller::Nintendo :
                (deviceType == 3 || deviceType == 6) ? Buttons::Controller::DualShock4 :
                Controllers::IsPlayStation(deviceType) ?
                Buttons::Controller::PlayStation : Buttons::Controller::Xbox;
            const auto image = GetPromptButtonImage(slot.kind, controller);
            void* nativeAtlas = image ? GetNativeAtlas(*image.atlas) : nullptr;
            if (nativeAtlas)
                return DrawButtonImage(nativeAtlas, image, x, y, width, height, clip, flags, colours,
                    sx, sy, renderMode, GetPromptArtworkScale(slot.kind));
            return originalDraw(texture, x, y, slot.original.source, sourceB, slot.original.width, height,
                clip, flags, colours, sx, sy, renderMode);
        }
    }
    return originalDraw(texture, x, y, sourceA, sourceB, width, height,
        clip, flags, colours, sx, sy, renderMode);
}
__declspec(noinline) static int __fastcall DrawHook(void* texture, void*,
    int x, int y, int sourceA, int sourceB, int width, int height,
    const void* clip, unsigned flags, const std::uint32_t* colours, float sx, float sy, int renderMode) {
    if (_ReturnAddress() != reinterpret_cast<void*>(0x004907F8))
        return originalDraw(texture,x,y,sourceA,sourceB,width,height,clip,flags,colours,sx,sy,renderMode);
    return RenderFontGlyph(texture,x,y,sourceA,sourceB,width,height,clip,flags,colours,sx,sy,renderMode);
}
inline bool Install() {
    char inlineOption[8] = {};
    if (GetEnvironmentVariableA("RETROINPUT_INLINE_GLYPHS",
        inlineOption, sizeof(inlineOption)) == 1 && inlineOption[0] == '0') {
        Log("Controller glyphs disabled; using text prompts");
        return true;
    }
    // Fixed-address game renderer: reject another image/build or a pre-existing draw hook.
    auto base = reinterpret_cast<unsigned char*>(GetModuleHandleW(nullptr));
    auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    if (reinterpret_cast<std::uintptr_t>(base) != 0x00400000 ||
        dos->e_magic != IMAGE_DOS_SIGNATURE) return false;
    auto pe = reinterpret_cast<IMAGE_NT_HEADERS32*>(base + dos->e_lfanew);
    if (pe->Signature != IMAGE_NT_SIGNATURE || pe->FileHeader.Machine != IMAGE_FILE_MACHINE_I386 ||
        pe->FileHeader.TimeDateStamp != 857672155 ||
        pe->OptionalHeader.SizeOfImage != 3538944 ||
        pe->OptionalHeader.AddressOfEntryPoint != 0x001D8426) {
        Log("Rejected executable build"); return false;
    }
    const unsigned char createBytes[] = { 0x56, 0x6A, 0x03, 0xE8 };
    const unsigned char drawBytes[] = { 0x81, 0xEC, 0xB0, 0, 0, 0 };
    const unsigned char callBytes[] = {0xFF,0x93,0x80,0,0,0};
    if (memcmp(reinterpret_cast<void*>(0x004907F2),callBytes,sizeof(callBytes)) ||
        memcmp(reinterpret_cast<void*>(0x0044E460), createBytes, sizeof(createBytes)) ||
        memcmp(reinterpret_cast<void*>(0x00450780), drawBytes, sizeof(drawBytes))) {
        Log("Rejected function signatures"); return false;
    }
    auto slot = reinterpret_cast<PVOID volatile*>(0x005EAFC8);
    originalDraw = reinterpret_cast<Draw>(0x00450780);
    DWORD protection = 0;
    if (!VirtualProtect(const_cast<PVOID*>(slot), sizeof(PVOID), PAGE_READWRITE, &protection)) {
        Log("Could not protect vtable slot"); return false;
    }
    PVOID previous = InterlockedCompareExchangePointer(slot,
        reinterpret_cast<PVOID>(&DrawHook), reinterpret_cast<PVOID>(originalDraw));
    DWORD ignored = 0;
    VirtualProtect(const_cast<PVOID*>(slot), sizeof(PVOID), protection, &ignored);
    if (previous != reinterpret_cast<PVOID>(originalDraw)) {
        Log("Rejected existing draw hook"); return false;
    }
    if (!InstallLookup()) {
        Log("Glyph rendering unavailable; ordinary prompt names retained");
        return false;
    }
    return true;
}
#else
inline bool InlineEnabled() { return false; }
inline bool Install() { return false; }
#endif
} // namespace RetroInput::TheSufferingTTB::ButtonRenderer

