#include <Windows.h>
#include "../RetroInput/Core/ButtonImages.h"
#include <cassert>
#include <cstring>
namespace RetroInput::Buttons {
#include "../RetroInput/Core/XboxSeriesImageMap.h"
#include "../RetroInput/Core/DualShock4ImageMap.h"
#include "../RetroInput/Core/NintendoImageMap.h"
}
int main(int argc, char** argv) {
    using namespace RetroInput::Buttons;
    SetEnvironmentVariableA("RETROINPUT_ICON_PACK", argc > 1 ? argv[1] : nullptr);
    const bool missing = argc > 1 && strcmp(argv[1], "missing") == 0;
    assert(InitializeButtonImages(missing ? GetModuleHandleW(L"kernel32.dll") : GetModuleHandleW(nullptr)) == !missing);
    const auto legacy = GetLegacyButtonImage(Controller::Xbox, Button::South).atlas;
    const auto canvas = GetButtonAtlasSize();
    assert(canvas.width >= legacy->width && canvas.height >= legacy->height);
    if (missing) assert(canvas.width == legacy->width && canvas.height == legacy->height);
    for (auto pack : {IconPack::DualSense, IconPack::XboxSeries, IconPack::DualShock4, IconPack::Nintendo}) {
        const auto decoded = GetButtonImage(pack, Button::South).atlas;
        assert(decoded->width <= canvas.width && decoded->height <= canvas.height);
        if (!missing) {
            std::uint32_t hash = 2166136261u;
            const auto bytes = reinterpret_cast<const unsigned char*>(decoded->pixels);
            for (int i = 0; i < decoded->strideBytes * decoded->height; ++i)
                hash = (hash ^ bytes[i]) * 16777619u;
            assert(hash == (pack == IconPack::DualSense ? 0x3378932Du : pack == IconPack::XboxSeries ? 0x87E2495Du : pack == IconPack::DualShock4 ? 0xCBCE8F24u : 0x82CFF24Bu));
        }
        for (unsigned i = 0; i < static_cast<unsigned>(Button::Count); ++i) {
            const int tile = pack == IconPack::DualSense ?
                (i < DualSenseButtonCount ? static_cast<int>(i) : -1) : pack == IconPack::XboxSeries ? XboxSeriesTiles[i] : pack == IconPack::DualShock4 ? DualShock4Tiles[i] : NintendoTiles[i];
            const auto image = GetButtonImage(pack, static_cast<Button>(i));
            if (tile < 0 || (missing && i >= LegacyButtonCount)) { assert(!image); continue; }
            assert(image);
            if (missing) { assert(image.atlas == legacy); continue; }
            assert(image.atlas->width == 1024 && image.atlas->height == 1024);
            assert(image.width == 128 && image.height == 128);
            assert(image.x == tile % 8 * 128 && image.y == tile / 8 * 128);
            unsigned visible = 0;
            for (int y = 0; y < 128; ++y) for (int x = 0; x < 128; ++x) {
                const auto pixel = image.atlas->pixels[(image.y+y)*1024+image.x+x];
                visible += (pixel >> 24) != 0;
                if (x < 4 || x >= 124 || y < 4 || y >= 124) assert((pixel >> 24) == 0);
            }
            assert(visible > 100);
        }
    }
    const auto nintendo = GetButtonImage(Controller::Nintendo, Button::South);
    const auto xbox = GetButtonImage(Controller::Xbox, Button::South);
    const auto ps4 = GetButtonImage(Controller::DualShock4, Button::South);
    const auto ps = GetButtonImage(Controller::PlayStation, Button::South);
    if (GetSelectedIconPack() == IconPack::Automatic) {
        assert((nintendo.atlas != xbox.atlas) == !missing);
        assert((xbox.atlas != legacy) == !missing);
        assert((ps.atlas != legacy) == !missing);
        assert((ps4.atlas != ps.atlas) == !missing);
        assert((ps4.atlas != xbox.atlas) == !missing);
        assert((xbox.atlas != ps.atlas) == !missing);
    } else {
        assert(nintendo.atlas == ps.atlas && nintendo.x == ps.x && nintendo.y == ps.y);
        assert(xbox.atlas == ps.atlas && xbox.x == ps.x && xbox.y == ps.y);
        assert(ps4.atlas == ps.atlas && ps4.x == ps.x && ps4.y == ps.y);
    }
    assert(!GetButtonImage(IconPack::DualSense, Button::Share));
    assert(!GetButtonImage(IconPack::XboxSeries, Button::Touchpad));
    assert(!GetButtonImage(IconPack::XboxSeries, Button::Count));
    return 0;
}
