#pragma once
#include <Windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "../../Core/AtlasUpload.h"

namespace RetroInput::TheSufferingTTB::RenderingSupport {
struct LockedRect { int pitch; void* pixels; };
inline bool BesideModule(const wchar_t* name, wchar_t (&path)[MAX_PATH])
{
    HMODULE module = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&BesideModule), &module)) return false;
    const DWORD length = GetModuleFileNameW(module, path, MAX_PATH);
    if (!length || length >= MAX_PATH) return false;
    wchar_t* slash = wcsrchr(path, L'\\');
    if (!slash) return false;
    return wcscpy_s(slash + 1, MAX_PATH - (slash + 1 - path), name) == 0;
}

inline void Log(const char* message, long value = 0)
{
    wchar_t path[MAX_PATH] = {};
    FILE* file = nullptr;
    if (BesideModule(L"RetroInputTTBImages.log", path))
        _wfopen_s(&file, path, L"a");
    if (!file) {
        wchar_t dir[MAX_PATH] = {};
        const DWORD length = GetTempPathW(MAX_PATH, dir);
        if (!length || length >= MAX_PATH ||
            swprintf_s(path, L"%sRetroInputTTBImages.log", dir) < 0) return;
        if (_wfopen_s(&file, path, L"a") != 0 || !file) return;
    }
    fprintf(file, "%lu %s (%ld)\n", GetTickCount(), message, value);
    fclose(file);
}

// Validate geometry before touching GPU memory. Shared pixels stay top-to-bottom;
// TTB's screen-rectangle path requires reversed rows at upload.
inline bool CopyPixels(const Buttons::AtlasImage& source, const LockedRect& destination,
    Buttons::AtlasSize canvas)
{
    return Buttons::CopyAtlasPixels(source, destination.pixels, destination.pitch,
        canvas, Buttons::AtlasRowOrder::BottomToTop);
}

} // namespace RetroInput::TheSufferingTTB::RenderingSupport
