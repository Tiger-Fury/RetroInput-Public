#pragma once
#include <cstdint>
namespace RetroInput::Controllers {
class TapHold {
public:
    struct State { bool tap = false, hold = false; };
    explicit TapHold(std::uint32_t threshold = 300) : threshold_(threshold) {}
    void Reset() { down_ = held_ = false; started_ = 0; }
    State Update(bool down, std::uint32_t now) {
        State result;
        if (down) {
            if (!down_) { started_ = now; held_ = false; }
            if (std::uint32_t(now - started_) >= threshold_) held_ = true;
            result.hold = held_;
        } else if (down_) {
            result.tap = !held_ && std::uint32_t(now-started_) < threshold_;
            held_ = false;
        }
        down_ = down;
        return result;
    }
private:
    std::uint32_t threshold_, started_ = 0;
    bool down_ = false, held_ = false;
};
} // namespace RetroInput::Controllers
