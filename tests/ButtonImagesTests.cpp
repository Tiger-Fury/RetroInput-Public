#include "../RetroInput/Core/ButtonImages.h"
#include <cassert>

int main() {
    using namespace RetroInput::Buttons;
    const auto& atlas = GetButtonAtlas();
    assert(atlas.width == 256 && atlas.height == 128 && atlas.strideBytes == 1024);
    assert(atlas.format == PixelFormat::StraightAlphaARGB32);
    for (auto controller : {Controller::Xbox, Controller::PlayStation}) {
        for (unsigned i = 0; i < LegacyButtonCount; ++i) {
            const auto image = GetLegacyButtonImage(controller, static_cast<Button>(i));
            assert(image && image.atlas == &atlas);
            assert(image.x >= 0 && image.y >= 0 && image.width > 0 && image.height > 0);
            assert(image.x + image.width <= atlas.width && image.y + image.height <= atlas.height);
            unsigned visible = 0, partial = 0;
            for (int y = image.y; y < image.y + image.height; ++y)
                for (int x = image.x; x < image.x + image.width; ++x) {
                    const auto alpha = atlas.pixels[y * atlas.width + x] >> 24;
                    visible += alpha != 0;
                    partial += alpha != 0 && alpha != 255;
                }
            assert(visible > 40 && partial > 0);
        }
    }
    const auto cross = GetLegacyButtonImage(Controller::PlayStation, Button::South);
    const auto a = GetLegacyButtonImage(Controller::Xbox, Button::South);
    assert(cross.x == 0 && cross.y == 0 && a.x == 128 && a.y == 0);
    const auto square = GetLegacyButtonImage(Controller::PlayStation, Button::West);
    assert(square.x == 64 && square.y == 0);
    assert(GetLegacyButtonImage(Controller::PlayStation, Button::Menu).x !=
        GetLegacyButtonImage(Controller::Xbox, Button::Menu).x);
    assert(!GetLegacyButtonImage(Controller::Xbox, Button::Count));
    assert(!GetLegacyButtonImage(Controller::Xbox, static_cast<Button>(-1)));
    assert(!GetLegacyButtonImage(static_cast<Controller>(99), Button::South));
    assert(GetButtonAtlas().pixels == atlas.pixels);
    assert(!GetLegacyButtonImage(Controller::Xbox, Button::Touchpad));
    const auto vertical = GetLegacyButtonImage(Controller::Xbox, Button::DpadUpDown);
    assert(vertical.x == 96 && vertical.y == 96 && vertical.width == 32 && vertical.height == 32);
    const auto psVertical = GetLegacyButtonImage(Controller::PlayStation, Button::DpadUpDown);
    assert(psVertical.x == vertical.x && psVertical.y == vertical.y);
    return 0;
}
