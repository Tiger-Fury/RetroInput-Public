#include "../RetroInput/Core/AtlasUpload.h"
#include <array>
#include <cassert>
#include <cstring>

using namespace RetroInput::Buttons;
int main() {
    // A larger pack followed by a smaller padded source must erase old artwork.
    std::array<std::uint32_t, 16> large;
    large.fill(0xFF112233);
    std::array<std::uint32_t, 24> target;
    target.fill(0xDEADBEEF); // Two pitch-padding pixels per destination row.
    const AtlasImage big{large.data(), 4, 4, 16, PixelFormat::StraightAlphaARGB32};
    assert(CopyAtlasPixels(big, target.data(), 24, {4, 4}, AtlasRowOrder::BottomToTop));
    const std::array<std::uint32_t, 6> small{1, 2, 99, 3, 4, 99};
    const AtlasImage little{small.data(), 2, 2, 12, PixelFormat::StraightAlphaARGB32};
    assert(CopyAtlasPixels(little, target.data(), 24, {4, 4}, AtlasRowOrder::BottomToTop));
    for (int y = 0; y < 4; ++y) {
        assert(target[y * 6 + 4] == 0xDEADBEEF && target[y * 6 + 5] == 0xDEADBEEF);
        for (int x = 0; x < 4; ++x) {
            const auto expected = y < 2 || x >= 2 ? 0u : small[(3 - y) * 3 + x];
            assert(target[y * 6 + x] == expected);
        }
    }
    assert(CopyAtlasPixels(little, target.data(), 24, {4, 4}, AtlasRowOrder::TopToBottom));
    assert(target[0] == 1 && target[1] == 2 && target[6] == 3 && target[7] == 4);
    assert(target[12] == 0 && target[18] == 0);
    const auto before = target;
    assert(!CopyAtlasPixels(big, target.data(), 12, {4, 4}, AtlasRowOrder::BottomToTop));
    assert(!CopyAtlasPixels(big, target.data(), 24, {2, 2}, AtlasRowOrder::BottomToTop));
    assert(!CopyAtlasPixels(big, target.data(), 24, {4097, 4}, AtlasRowOrder::BottomToTop));
    assert(!CopyAtlasPixels(little, nullptr, 24, {4, 4}, AtlasRowOrder::BottomToTop));
    AtlasImage invalid = little;
    invalid.strideBytes = 4;
    assert(!CopyAtlasPixels(invalid, target.data(), 24, {4, 4}, AtlasRowOrder::BottomToTop));
    assert(target == before);
}
