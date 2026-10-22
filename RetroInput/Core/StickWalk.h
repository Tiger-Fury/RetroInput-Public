#pragma once
#include <cmath>

namespace RetroInput::Controllers {
// Raw signed-stick coordinates; thresholds precede any deadzone rescaling.
inline float LeftStickMagnitude(short x, short y)
{
    const float nx = x / (x < 0 ? 32768.0f : 32767.0f);
    const float ny = y / (y < 0 ? 32768.0f : 32767.0f);
    const float magnitude = std::sqrt(nx * nx + ny * ny);
    return magnitude > 1.0f ? 1.0f : magnitude;
}

class StickWalk {
public:
    bool Update(short x, short y)
    {
        const float magnitude = LeftStickMagnitude(x, y);
        if (magnitude <= 0.15f) { Reset(); return false; }
        if (!moving_) walking_ = magnitude <= 0.48f;
        else if (magnitude > 0.48f) walking_ = false;
        else if (magnitude < 0.42f) walking_ = true;
        moving_ = true;
        return walking_;
    }
    bool Moving() const { return moving_; }
    void Reset() { moving_ = walking_ = false; }
private:
    bool moving_ = false;
    bool walking_ = false;
};
} // namespace RetroInput::Controllers
