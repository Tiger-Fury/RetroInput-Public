#pragma once
#include "ButtonImages.h"
#include <cstddef>
#include <cstring>

namespace RetroInput::Buttons {
enum class AtlasRowOrder { TopToBottom, BottomToTop };

// Upload an individual pack into a reusable canvas without changing its UVs.
// Clear unused pixels when moving from a larger pack to a smaller one.
inline bool CopyAtlasPixels(const AtlasImage& image, void* pixels, int pitch,
    AtlasSize canvas, AtlasRowOrder order) {
    if (!pixels || !image.pixels || image.format != PixelFormat::StraightAlphaARGB32 ||
        canvas.width <= 0 || canvas.height <= 0 || canvas.width > 4096 || canvas.height > 4096 ||
        image.width <= 0 || image.height <= 0 || image.width > canvas.width || image.height > canvas.height ||
        image.strideBytes < image.width * 4 || pitch < canvas.width * 4) return false;
    auto target = static_cast<unsigned char*>(pixels);
    const auto source = reinterpret_cast<const unsigned char*>(image.pixels);
    for (int y = 0; y < canvas.height; ++y) {
        auto row = target + static_cast<std::size_t>(y) * pitch;
        const int sourceY = order == AtlasRowOrder::BottomToTop ? canvas.height - 1 - y : y;
        if (sourceY < image.height) {
            memcpy(row, source + static_cast<std::size_t>(sourceY) * image.strideBytes,
                static_cast<std::size_t>(image.width) * 4);
            memset(row + image.width * 4, 0, static_cast<std::size_t>(canvas.width - image.width) * 4);
        } else {
            memset(row, 0, static_cast<std::size_t>(canvas.width) * 4);
        }
    }
    return true;
}
} // namespace RetroInput::Buttons
