#pragma once
#include <Windows.h>
#include <Xinput.h>
#include "Core/ControllerActivity.h"
#include "DualSense.h"
#include "DualShock4.h"
#include "Shield.h"
#include "Backbone.h"
#include "Vx2.h"
#include "JoyCon.h"
#include "Core/XInputAggregator.h"

// Game-independent device activity arbitration. Preserves the original
// 30-second selection window; the most recently active device keeps selection.
namespace RetroController {
struct Selection {
    int type = 0; // 0=keyboard, 1=Xbox, 2=DualSense, 3=DualShock4, 4=SHIELD, 5=Backbone, 6=VX2, 7=Joy-Con pair
    XINPUT_GAMEPAD gamepad = {};
};

class ControllerSelector {
public:
    Selection Poll(bool physicalKeyboard) {
        std::array<XINPUT_STATE, XUSER_MAX_COUNT> states = {};
        std::array<bool, XUSER_MAX_COUNT> connected = {};
        for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i)
            connected[i] = XInputGetState(i, &states[i]) == ERROR_SUCCESS;
        const auto xbox = xbox_.Update(states, connected);
        const bool xboxConnected = xbox.connected;
        const bool xboxActivity = xboxConnected && RetroInput::Controllers::HasActivity(xbox.gamepad);
        if (xboxActivity) lastXboxActivity_ = GetTickCount();

        const DualSenseSnapshot ps = GetDualSenseSnapshot();
        const DualShock4Snapshot ps4 = GetDualShock4Snapshot();
        const ShieldSnapshot shield = GetShieldSnapshot();
        const BackboneSnapshot backbone = GetBackboneSnapshot();
        const Vx2Snapshot vx2 = GetVx2Snapshot();
        const JoyConSnapshot joycon = GetJoyConSnapshot();
        const DWORD now = GetTickCount();
        const bool dsRecent = ps.connected && ps.lastActivity != 0 &&
            static_cast<DWORD>(now - ps.lastActivity) < 30000;
        const bool ds4Recent = ps4.connected && ps4.lastActivity != 0 &&
            static_cast<DWORD>(now - ps4.lastActivity) < 30000;
        const bool usePs4 = ds4Recent && (!dsRecent ||
            static_cast<LONG>(ps4.lastActivity - ps.lastActivity) > 0);
        const DWORD psTime = usePs4 ? ps4.lastActivity : ps.lastActivity;
        const bool psRecent = dsRecent || ds4Recent;
        const int sonyType = usePs4 ? 3 : 2;
        const bool xboxRecent = xboxConnected && lastXboxActivity_ != 0 &&
            static_cast<DWORD>(now - lastXboxActivity_) < 30000;

        int selected = 0;
        if (psRecent && (!xboxRecent ||
            static_cast<LONG>(psTime - lastXboxActivity_) > 0))
            selected = sonyType;
        else if (xboxRecent)
            selected = 1;

        const bool shieldRecent = shield.connected && shield.lastActivity != 0 &&
            static_cast<DWORD>(now - shield.lastActivity) < 30000;
        const DWORD selectedTime = selected == 1 ? lastXboxActivity_ : psTime;
        if (shieldRecent && (selected == 0 ||
            static_cast<LONG>(shield.lastActivity - selectedTime) > 0))
            selected = 4;

        const bool backboneRecent = backbone.connected && backbone.lastActivity != 0 &&
            static_cast<DWORD>(now - backbone.lastActivity) < 30000;
        const DWORD currentTime = selected == 4 ? shield.lastActivity : selectedTime;
        if (backboneRecent && (selected == 0 ||
            static_cast<LONG>(backbone.lastActivity - currentTime) > 0))
            selected = 5;

        const bool vx2Recent = vx2.connected && vx2.lastActivity != 0 &&
            static_cast<DWORD>(now - vx2.lastActivity) < 30000;
        const DWORD latestTime = selected == 5 ? backbone.lastActivity : currentTime;
        if(vx2Recent && (selected == 0 || static_cast<LONG>(vx2.lastActivity-latestTime)>0))
            selected = 6;

        const bool joyconRecent = joycon.connected && joycon.lastActivity != 0 &&
            static_cast<DWORD>(now - joycon.lastActivity) < 30000;
        const DWORD newestTime = selected == 6 ? vx2.lastActivity : latestTime;
        if (joyconRecent && (selected == 0 ||
            static_cast<LONG>(joycon.lastActivity - newestTime) > 0)) selected = 7;

        if (physicalKeyboard) {
            lastXboxActivity_ = 0;
            xbox_.Reset();
            ResetDualSenseActivity();
            ResetDualShock4Activity();
            ResetShieldActivity();
            ResetBackboneActivity();
            ResetVx2Activity();
            ResetJoyConActivity();
            return {};
        }
        if (selected == 7) return { 7, joycon.gamepad };
        if (selected == 6) return { 6, vx2.gamepad };
        if (selected == 5) return { 5, backbone.gamepad };
        if (selected == 4) return { 4, shield.gamepad };
        if (selected == 1) return { 1, xbox.gamepad };
        if (selected == 2 && ps.connected) return { 2, ps.gamepad };
        if (selected == 3 && ps4.connected) return { 3, ps4.gamepad };
        return {};
    }

private:
    DWORD lastXboxActivity_ = 0;
    XInputAggregator xbox_;
};
} // namespace RetroController
