#include "pch.h"
#include "ButtonImages.h"
#include <wincodec.h>
#include <wrl/client.h>
#include <vector>
#include <mutex>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "windowscodecs.lib")

namespace RetroInput::Buttons {
#include "XboxSeriesImageMap.h"
#include "DualShock4ImageMap.h"
#include "NintendoImageMap.h"
namespace {
using Microsoft::WRL::ComPtr;
std::once_flag initialized;
struct ImagePack {
    std::vector<std::uint32_t> pixels;
    AtlasImage atlas = {};
    bool available = false;
};
ImagePack dualSense, xboxSeries, dualShock4, nintendo;
IconPack selected = IconPack::Automatic;

bool LoadPack(HMODULE module, unsigned resourceId, unsigned tileCount, ImagePack& pack) {
    const HRSRC resource = FindResourceW(module, MAKEINTRESOURCEW(resourceId), MAKEINTRESOURCEW(10));
    if (!resource) return false;
    const DWORD size = SizeofResource(module, resource);
    const HGLOBAL loaded = LoadResource(module, resource);
    auto bytes = static_cast<BYTE*>(LockResource(loaded));
    if (!size || !bytes) return false;
    const HRESULT apartment = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(apartment) && apartment != RPC_E_CHANGED_MODE) return false;
    struct Apartment {
        bool owned;
        ~Apartment() { if (owned) CoUninitialize(); }
    } scope{SUCCEEDED(apartment)};
    ComPtr<IWICImagingFactory> factory;
    ComPtr<IWICStream> stream;
    ComPtr<IWICBitmapDecoder> decoder;
    ComPtr<IWICBitmapFrameDecode> frame;
    ComPtr<IWICFormatConverter> converter;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(factory.GetAddressOf()))) ||
        FAILED(factory->CreateStream(&stream)) ||
        FAILED(stream->InitializeFromMemory(bytes, size)) ||
        FAILED(factory->CreateDecoderFromStream(stream.Get(), nullptr, WICDecodeMetadataCacheOnLoad, &decoder)) ||
        FAILED(decoder->GetFrame(0, &frame))) return false;
    UINT width = 0, height = 0;
    if (FAILED(frame->GetSize(&width, &height)) || !width || width > 4096 ||
        width % 8 || height != width ||
        (height / (width / 8)) * 8 < tileCount) return false;
    if (FAILED(factory->CreateFormatConverter(&converter)) ||
        FAILED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppBGRA,
            WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom))) return false;
    std::vector<std::uint32_t> pixels(static_cast<size_t>(width) * height);
    if (FAILED(converter->CopyPixels(nullptr, width * 4,
        static_cast<UINT>(pixels.size() * 4), reinterpret_cast<BYTE*>(pixels.data())))) return false;
    pack.pixels = std::move(pixels);
    pack.atlas = {pack.pixels.data(), static_cast<int>(width), static_cast<int>(height),
        static_cast<int>(width * 4), PixelFormat::StraightAlphaARGB32};
    return true;
}
}

bool InitializeButtonImages(void* module) {
    std::call_once(initialized, [module] {
        char option[32] = {};
        const DWORD length = GetEnvironmentVariableA("RETROINPUT_ICON_PACK", option, sizeof(option));
        if (length && length < sizeof(option)) {
            if (_stricmp(option, "dualsense") == 0) selected = IconPack::DualSense;
            else if (_stricmp(option, "xboxseries") == 0 || _stricmp(option, "xbox-series") == 0)
                selected = IconPack::XboxSeries;
            else if (_stricmp(option, "dualshock4") == 0 || _stricmp(option, "ps4") == 0)
                selected = IconPack::DualShock4;
            else if (_stricmp(option, "nintendo") == 0 || _stricmp(option, "switch") == 0)
                selected = IconPack::Nintendo;
            else if (_stricmp(option, "xbox") == 0) selected = IconPack::Xbox;
            else if (_stricmp(option, "playstation") == 0) selected = IconPack::PlayStation;
        }
        dualSense.available = LoadPack(static_cast<HMODULE>(module), 201, DualSenseButtonCount, dualSense);
        dualShock4.available = LoadPack(static_cast<HMODULE>(module), 203, 58, dualShock4);
        nintendo.available = LoadPack(static_cast<HMODULE>(module), 204, 45, nintendo);
        xboxSeries.available = LoadPack(static_cast<HMODULE>(module), 202, 38, xboxSeries);
    });
    return dualSense.available && xboxSeries.available && dualShock4.available && nintendo.available;
}
IconPack GetSelectedIconPack() { return selected; }
AtlasSize GetButtonAtlasSize() {
    const auto& legacy = GetButtonAtlas();
    AtlasSize size{legacy.width, legacy.height};
    for (const auto* pack : {&dualSense, &dualShock4, &nintendo, &xboxSeries}) {
        if (!pack->available) continue;
        if (pack->atlas.width > size.width) size.width = pack->atlas.width;
        if (pack->atlas.height > size.height) size.height = pack->atlas.height;
    }
    return size;
}

ButtonImage GetButtonImage(IconPack pack, Button button) {
    if (static_cast<unsigned>(button) >= static_cast<unsigned>(Button::Count)) return {};
    if (pack == IconPack::DualSense) {
        if (static_cast<unsigned>(button) >= DualSenseButtonCount) return {};
        if (!dualSense.available) return GetLegacyButtonImage(Controller::PlayStation, button);
        const auto index = static_cast<unsigned>(button);
        const int tileSize = dualSense.atlas.width / 8;
        return {&dualSense.atlas, static_cast<int>(index % 8) * tileSize,
            static_cast<int>(index / 8) * tileSize, tileSize, tileSize};
    }
    if (pack == IconPack::DualShock4) {
        const int tile = DualShock4Tiles[static_cast<unsigned>(button)];
        if (tile < 0) return {};
        if (!dualShock4.available) return GetLegacyButtonImage(Controller::PlayStation, button);
        const int tileSize = dualShock4.atlas.width / 8;
        return {&dualShock4.atlas, tile % 8 * tileSize, tile / 8 * tileSize, tileSize, tileSize};
    }
    if (pack == IconPack::XboxSeries) {
        const int tile = XboxSeriesTiles[static_cast<unsigned>(button)];
        if (tile < 0) return {};
        if (!xboxSeries.available) return GetLegacyButtonImage(Controller::Xbox, button);
        const int tileSize = xboxSeries.atlas.width / 8;
        return {&xboxSeries.atlas, tile % 8 * tileSize, tile / 8 * tileSize, tileSize, tileSize};
    }
    if (pack == IconPack::Nintendo) {
        const int tile = NintendoTiles[static_cast<unsigned>(button)];
        if (tile < 0) return {};
        if (!nintendo.available) return GetLegacyButtonImage(Controller::Xbox, button);
        const int tileSize = nintendo.atlas.width / 8;
        return {&nintendo.atlas, tile % 8 * tileSize, tile / 8 * tileSize, tileSize, tileSize};
    }
    if (pack == IconPack::Xbox) return GetLegacyButtonImage(Controller::Xbox, button);
    if (pack == IconPack::PlayStation) return GetLegacyButtonImage(Controller::PlayStation, button);
    return {}; // Automatic requires a controller context.
}
ButtonImage GetButtonImage(Controller controller, Button button) {
    if (controller != Controller::Xbox && controller != Controller::PlayStation && controller != Controller::DualShock4 && controller != Controller::Nintendo) return {};
    const IconPack pack = selected == IconPack::Automatic ?
        (controller == Controller::Nintendo ? IconPack::Nintendo :
         controller == Controller::DualShock4 ? IconPack::DualShock4 :
         controller == Controller::PlayStation ? IconPack::DualSense : IconPack::XboxSeries) : selected;
    return GetButtonImage(pack, button);
}
} // namespace RetroInput::Buttons
