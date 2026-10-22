#pragma once
#include <array>
#include <cstdint>
#include "ButtonImageData.h"

// Engine-independent image service. Views and pixels remain valid for process lifetime.
// Pixels are top-to-bottom, straight-alpha 0xAARRGGBB; adapters own GPU resources.
namespace RetroInput::Buttons {
enum class Controller { Xbox, PlayStation, DualShock4, Nintendo };
// Artwork choice is independent of physical-device support.
enum class IconPack { Automatic, Xbox, PlayStation, DualSense, XboxSeries, DualShock4, Nintendo };
enum class Button {
    South, East, West, North, LeftShoulder, RightShoulder, LeftTrigger, RightTrigger,
    DpadUp, DpadDown, DpadLeft, DpadRight, DpadLeftRight, Menu, View, LeftStick, RightStick,
    DpadUpDown,
    LeftStickClick, LeftStickClickAllDirections, LeftStickClickLeftRight, LeftStickClickUpDown,
    LeftStickClickUp, LeftStickClickDown, LeftStickClickLeft, LeftStickClickRight,
    RightStickClick, RightStickClickAllDirections, RightStickClickLeftRight, RightStickClickUpDown,
    RightStickClickUp, RightStickClickDown, RightStickClickLeft, RightStickClickRight,
    LeftStickAllDirections, LeftStickLeftRight, LeftStickUpDown,
    LeftStickUp, LeftStickDown, LeftStickLeft, LeftStickRight,
    RightStickAllDirections, RightStickLeftRight, RightStickUpDown,
    RightStickUp, RightStickDown, RightStickLeft, RightStickRight,
    DpadNeutral, Touchpad,
    LeftStickClickDiagram, RightStickClickDiagram, RightStickClickDiagramAlt, LeftStickClickDiagramAlt,
    LeftStickDiagram, RightStickDiagram, RightStickDiagramAlt, LeftStickDiagramAlt,
    Share, Home, LeftRailShoulder, RightRailShoulder,
    JoyConPairDiagram, JoyConDiagram, JoyConDiagramAlt, JoyConPairDiagramAlt,
    DpadUpDiagram, DpadLeftDiagram, DpadDownDiagram, DpadRightDiagram, Count
};
inline constexpr unsigned LegacyButtonCount = 18;
inline constexpr unsigned DualSenseButtonCount = 58;
enum class PixelFormat { StraightAlphaARGB32 };
struct AtlasImage {
    const std::uint32_t* pixels;
    int width, height, strideBytes;
    PixelFormat format;
};
struct ButtonImage {
    const AtlasImage* atlas = nullptr;
    int x = 0, y = 0, width = 0, height = 0;
    explicit operator bool() const { return atlas != nullptr; }
};
inline const AtlasImage& GetButtonAtlas() {
    static const auto pixels = [] {
        std::array<std::uint32_t, Data::Width * Data::Height> result = {};
        unsigned index = 0;
        for (const auto& run : Data::Runs)
            for (unsigned i = 0; i < run.count; ++i) result[index++] = run.argb;
        return result;
    }();
    static const AtlasImage image = {
        pixels.data(), Data::Width, Data::Height, Data::Width * 4,
        PixelFormat::StraightAlphaARGB32
    };
    return image;
}
inline ButtonImage GetLegacyButtonImage(Controller controller, Button button) {
    // Image-pack layout is private to this service, never a game-adapter contract.
    constexpr unsigned xbox[] = {4,5,6,7,12,13,14,15,16,17,18,19,20,23,24,25,26,27};
    constexpr unsigned playStation[] = {0,1,2,3,8,9,10,11,16,17,18,19,20,21,22,25,26,27};
    static_assert(sizeof(xbox) / sizeof(xbox[0]) == LegacyButtonCount);
    static_assert(sizeof(playStation) / sizeof(playStation[0]) == LegacyButtonCount);
    const auto index = static_cast<unsigned>(button);
    if (index >= LegacyButtonCount ||
        (controller != Controller::Xbox && controller != Controller::PlayStation)) return {};
    const auto tile = controller == Controller::PlayStation ? playStation[index] : xbox[index];
    return { &GetButtonAtlas(), static_cast<int>(tile % 8) * Data::TileSize,
        static_cast<int>(tile / 8) * Data::TileSize, Data::TileSize, Data::TileSize };
}
// Call once on a startup worker, outside DllMain and before installing render hooks.
// module is the module containing ButtonImages.rc. Failure retains legacy artwork.
bool InitializeButtonImages(void* module);
IconPack GetSelectedIconPack();
ButtonImage GetButtonImage(IconPack pack, Button button);
ButtonImage GetButtonImage(Controller controller, Button button);
} // namespace RetroInput::Buttons
