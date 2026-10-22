#pragma once
namespace RetroInput::Controllers {
enum class DeviceType : int { Keyboard = 0, Xbox = 1, DualSense = 2, DualShock4 = 3, Shield = 4, Backbone = 5, Vx2 = 6, JoyCon = 7 };
inline bool IsPlayStation(int type) {
    return type == static_cast<int>(DeviceType::DualSense) ||
        type == static_cast<int>(DeviceType::DualShock4) ||
        type == static_cast<int>(DeviceType::Backbone) ||
        type == static_cast<int>(DeviceType::Vx2);
}
}
