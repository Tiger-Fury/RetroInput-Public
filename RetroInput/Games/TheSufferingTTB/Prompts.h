#pragma once
#include <atomic>
#include <cstring>
#include "PromptText.h"
#include "RenderingSupport.h"
extern std::atomic<bool> g_controllerActive;
extern std::atomic<int> g_controllerType;
extern "C" BOOL AreInlineGlyphsEnabled();
namespace RetroInput::TheSufferingTTB::Prompts {
using Expand = wchar_t* (__thiscall*)(void*,const wchar_t*,int);
static Expand originalExpand = nullptr;
static bool installed = false;
static wchar_t* __fastcall ExpandHook(void* object,void*,const wchar_t* text,int context) {
    if (!originalExpand) return nullptr;
    if (!text || !g_controllerActive.load()) return originalExpand(object,text,context);
    try {
        const auto modified=PromptText::Transform(text,g_controllerType.load(),AreInlineGlyphsEnabled()!=FALSE);
        // The game's expander allocates the returned string with its own allocator.
        return originalExpand(object,modified.c_str(),context);
    } catch (...) { return originalExpand(object,text,context); }
}
inline bool Install() {
    if (installed) return true;
    auto target=reinterpret_cast<BYTE*>(0x0040E8D0);
    const BYTE expected[6]={0x81,0xEC,0x84,0,0,0};
    if (memcmp(target,expected,6)) {RenderingSupport::Log("Prompt entry signature rejected");return false;}
    auto trampoline=static_cast<BYTE*>(VirtualAlloc(nullptr,11,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    if (!trampoline) return false;
    memcpy(trampoline,expected,6);trampoline[6]=0xE9;
    const auto back=static_cast<std::int32_t>(reinterpret_cast<std::intptr_t>(target+6)-
        reinterpret_cast<std::intptr_t>(trampoline+11));
    memcpy(trampoline+7,&back,4);FlushInstructionCache(GetCurrentProcess(),trampoline,11);
    DWORD protection=0;
    if (!VirtualProtect(target,6,PAGE_EXECUTE_READWRITE,&protection)) {VirtualFree(trampoline,0,MEM_RELEASE);return false;}
    originalExpand=reinterpret_cast<Expand>(trampoline);
    BYTE patch[6]={0xE9,0,0,0,0,0x90};
    const auto jump=static_cast<std::int32_t>(reinterpret_cast<std::intptr_t>(&ExpandHook)-
        reinterpret_cast<std::intptr_t>(target+5));
    memcpy(patch+1,&jump,4);memcpy(target,patch,6);FlushInstructionCache(GetCurrentProcess(),target,6);
    DWORD unused=0;VirtualProtect(target,6,protection,&unused);
    installed=true;RenderingSupport::Log("Installed TTB controller prompt expansion");return true;
}
} // namespace RetroInput::TheSufferingTTB::Prompts
