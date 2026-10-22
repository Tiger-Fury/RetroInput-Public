#include <Windows.h>
#include <Xinput.h>
#include "../RetroInput/DualSense.h"
#include "../RetroInput/DualShock4.h"
#include "../RetroInput/Shield.h"
#include "../RetroInput/Backbone.h"
#include "../RetroInput/Vx2.h"
#include "../RetroInput/JoyCon.h"
#include <array>
#include <cassert>

static std::array<XINPUT_STATE, XUSER_MAX_COUNT> pads;
static std::array<bool, XUSER_MAX_COUNT> connectedPads;
static std::array<unsigned, XUSER_MAX_COUNT> calls;
static DWORD clockTime = 100;
static DualSenseSnapshot sony;
static DualShock4Snapshot sony4;
static ShieldSnapshot shieldSample;
static BackboneSnapshot backboneSample;
static Vx2Snapshot vx2Sample;
static JoyConSnapshot joyconSample;
static unsigned joyconResets = 0;
static JoyConSnapshot FakeJoyCon() { return joyconSample; }
static void FakeJoyConReset() { ++joyconResets; joyconSample.lastActivity = 0; }
static unsigned vx2Resets = 0;
static Vx2Snapshot FakeVx2() { return vx2Sample; }
static void FakeVx2Reset() { ++vx2Resets; vx2Sample.lastActivity = 0; }
static unsigned backboneResets = 0;
static BackboneSnapshot FakeBackbone() { return backboneSample; }
static void FakeBackboneReset() { ++backboneResets; backboneSample.lastActivity = 0; }
static unsigned shieldResets = 0;
static ShieldSnapshot FakeShield() { return shieldSample; }
static void FakeShieldReset() { ++shieldResets; shieldSample.lastActivity = 0; }
static unsigned resets4 = 0;
static unsigned resets = 0;
static DWORD FakeTick() { return clockTime; }
static DWORD FakeGet(DWORD index, XINPUT_STATE* state) {
    assert(index < XUSER_MAX_COUNT);
    ++calls[index];
    *state = pads[index];
    return connectedPads[index] ? ERROR_SUCCESS : ERROR_DEVICE_NOT_CONNECTED;
}
static DualSenseSnapshot FakeSony() { return sony; }
static void FakeReset() { ++resets; sony.lastActivity = 0; }
static DualShock4Snapshot FakeSony4() { return sony4; }
static void FakeReset4() { ++resets4; sony4.lastActivity = 0; }
#define GetJoyConSnapshot FakeJoyCon
#define ResetJoyConActivity FakeJoyConReset
#define GetVx2Snapshot FakeVx2
#define ResetVx2Activity FakeVx2Reset
#define GetBackboneSnapshot FakeBackbone
#define ResetBackboneActivity FakeBackboneReset
#define GetShieldSnapshot FakeShield
#define ResetShieldActivity FakeShieldReset
#define XInputGetState FakeGet
#define GetTickCount FakeTick
#define GetDualSenseSnapshot FakeSony
#define ResetDualSenseActivity FakeReset
#define GetDualShock4Snapshot FakeSony4
#define ResetDualShock4Activity FakeReset4
#include "../RetroInput/ControllerSelector.h"
#undef GetJoyConSnapshot
#undef ResetJoyConActivity
#undef GetVx2Snapshot
#undef ResetVx2Activity
#undef GetBackboneSnapshot
#undef ResetBackboneActivity
#undef GetShieldSnapshot
#undef ResetShieldActivity
#undef XInputGetState
#undef GetTickCount
#undef GetDualSenseSnapshot
#undef ResetDualSenseActivity
#undef GetDualShock4Snapshot
#undef ResetDualShock4Activity

int main() {
    using namespace RetroController;
    XInputAggregator merger;
    connectedPads[3] = true;
    pads[3].Gamepad.wButtons = XINPUT_GAMEPAD_A;
    auto sample = merger.Update(pads, connectedPads);
    assert(sample.connected && sample.gamepad.wButtons == XINPUT_GAMEPAD_A);
    connectedPads.fill(true);
    pads[0].Gamepad.wButtons = XINPUT_GAMEPAD_B;
    pads[1].Gamepad.wButtons = XINPUT_GAMEPAD_X | XINPUT_GAMEPAD_LEFT_THUMB;
    pads[2].Gamepad.wButtons = XINPUT_GAMEPAD_Y | XINPUT_GAMEPAD_RIGHT_THUMB;
    pads[0].Gamepad.bLeftTrigger = 80;
    pads[2].Gamepad.bLeftTrigger = 200;
    pads[1].Gamepad.bRightTrigger = 220;
    sample = merger.Update(pads, connectedPads);
    assert(sample.gamepad.wButtons == (XINPUT_GAMEPAD_A | XINPUT_GAMEPAD_B | XINPUT_GAMEPAD_X |
        XINPUT_GAMEPAD_Y | XINPUT_GAMEPAD_LEFT_THUMB | XINPUT_GAMEPAD_RIGHT_THUMB));
    assert(sample.gamepad.bLeftTrigger == 200 && sample.gamepad.bRightTrigger == 220);
    pads[3].Gamepad.wButtons = 0;
    assert(!(merger.Update(pads, connectedPads).gamepad.wButtons & XINPUT_GAMEPAD_A));
    connectedPads[2] = false;
    sample = merger.Update(pads, connectedPads);
    assert(!(sample.gamepad.wButtons & (XINPUT_GAMEPAD_Y | XINPUT_GAMEPAD_RIGHT_THUMB)));
    assert(sample.gamepad.bLeftTrigger == 80 && sample.gamepad.bRightTrigger == 220);

    pads = {};
    connectedPads.fill(true);
    merger.Reset();
    pads[0].Gamepad.sThumbLX = 20000;
    assert(merger.Update(pads, connectedPads).gamepad.sThumbLX == 20000);
    pads[1].Gamepad.sThumbRX = -24000;
    sample = merger.Update(pads, connectedPads);
    assert(sample.gamepad.sThumbLX == 0 && sample.gamepad.sThumbRX == -24000);
    pads[0].Gamepad.sThumbLX = 20100;
    pads[0].Gamepad.sThumbRY = 8000; // Below the camera deadzone.
    sample = merger.Update(pads, connectedPads);
    assert(sample.gamepad.sThumbRX == -24000);
    pads[0].Gamepad.sThumbRY = -8000; // Neutral jitter must not reclaim ownership.
    assert(merger.Update(pads, connectedPads).gamepad.sThumbRX == -24000);
    pads[0].Gamepad.sThumbLX = 23000;
    sample = merger.Update(pads, connectedPads);
    assert(sample.gamepad.sThumbLX == 23000 && sample.gamepad.sThumbRX == 0);
    connectedPads[0] = false;
    assert(merger.Update(pads, connectedPads).gamepad.sThumbRX == -24000);
    connectedPads[0] = true;
    assert(merger.Update(pads, connectedPads).gamepad.sThumbLX == 23000);
    pads[0] = {};
    assert(merger.Update(pads, connectedPads).gamepad.sThumbRX == -24000);
    pads[1] = {};
    assert(merger.Update(pads, connectedPads).gamepad.sThumbRX == 0);
    connectedPads.fill(false);
    assert(!merger.Update(pads, connectedPads).connected);

    ControllerSelector selector;
    pads = {}; connectedPads[2] = true;
    pads[2].Gamepad.wButtons = XINPUT_GAMEPAD_A;
    auto selection = selector.Poll(false);
    assert(selection.type == 1 && selection.gamepad.wButtons == XINPUT_GAMEPAD_A);
    for (auto count : calls) assert(count == 1);
    pads[2] = {};
    clockTime = 200;
    assert(selector.Poll(false).type == 1); // Recent Xbox input retains its prompt family.
    sony.connected = true; sony.lastActivity = 201;
    sony.gamepad.wButtons = XINPUT_GAMEPAD_B;
    clockTime = 202;
    assert(selector.Poll(false).type == 2);
    sony4.connected = true; sony4.lastActivity = 202;
    sony4.gamepad.wButtons = XINPUT_GAMEPAD_Y;
    assert(selector.Poll(false).type == 3);
    assert(selector.Poll(false).gamepad.wButtons == XINPUT_GAMEPAD_Y);
    clockTime = 203; sony.lastActivity = 203;
    assert(selector.Poll(false).type == 2);
    assert(selector.Poll(true).type == 0 && resets == 1);
    assert(resets4 == 1);
    assert(selector.Poll(false).type == 0);
    sony4.lastActivity = clockTime;
    assert(selector.Poll(false).type == 3);
    sony4.connected = false;
    assert(selector.Poll(false).type == 0);
    connectedPads[3] = true; pads[3].Gamepad.wButtons = XINPUT_GAMEPAD_X;
    assert(selector.Poll(false).type == 1);
    connectedPads.fill(false);
    assert(selector.Poll(false).type == 0);

    // Older Sony activity must not reclaim selection when Xbox is released.
    // Exercise each Sony reader independently, neutral jitter, later Sony use,
    // a second Xbox takeover, disconnect fallback and timeout.
    for (int sonyType : {2,3}) {
        ControllerSelector recent;
        pads = {}; connectedPads.fill(false); connectedPads[0] = true;
        sony = {}; sony4 = {};
        clockTime = 1000;
        if (sonyType == 2) { sony.connected = true; sony.lastActivity = clockTime; }
        else { sony4.connected = true; sony4.lastActivity = clockTime; }
        assert(recent.Poll(false).type == sonyType);
        clockTime = 1010; pads[0].Gamepad.wButtons = XINPUT_GAMEPAD_A;
        assert(recent.Poll(false).type == 1);
        pads[0] = {}; clockTime = 1011;
        assert(recent.Poll(false).type == 1);
        clockTime = 1500; pads[0].Gamepad.sThumbLX = 100;
        assert(recent.Poll(false).type == 1);
        clockTime = 2000;
        if (sonyType == 2) sony.lastActivity = clockTime;
        else sony4.lastActivity = clockTime;
        assert(recent.Poll(false).type == sonyType);
        clockTime = 2010; pads[0].Gamepad.wButtons = XINPUT_GAMEPAD_X;
        assert(recent.Poll(false).type == 1);
        pads[0] = {}; clockTime = 2011;
        assert(recent.Poll(false).type == 1);
        connectedPads[0] = false;
        assert(recent.Poll(false).type == sonyType);
        clockTime = 32000;
        assert(recent.Poll(false).type == 0);
    }
    sony = {}; sony4 = {};

    // SHIELD remains a distinct identity; last input wins across all families.
    ControllerSelector withShield;
    pads = {}; connectedPads.fill(false);
    sony = {}; sony4 = {}; shieldSample = {};
    clockTime = 5000; shieldSample.connected = true; shieldSample.lastActivity = clockTime;
    shieldSample.gamepad.wButtons = XINPUT_GAMEPAD_X;
    assert(withShield.Poll(false).type == 4);
    assert(withShield.Poll(false).gamepad.wButtons == XINPUT_GAMEPAD_X);
    clockTime = 5010; connectedPads[1] = true; pads[1].Gamepad.wButtons = XINPUT_GAMEPAD_A;
    assert(withShield.Poll(false).type == 1);
    pads[1] = {}; clockTime = 5011; shieldSample.gamepad = {};
    assert(withShield.Poll(false).type == 1);
    clockTime = 5020; shieldSample.lastActivity = clockTime;
    assert(withShield.Poll(false).type == 4);
    clockTime = 5030; sony.connected = true; sony.lastActivity = clockTime;
    assert(withShield.Poll(false).type == 2);
    clockTime = 5040; shieldSample.lastActivity = clockTime;
    assert(withShield.Poll(false).type == 4);
    clockTime = 5050; sony4.connected = true; sony4.lastActivity = clockTime;
    assert(withShield.Poll(false).type == 3);
    clockTime = 5060; shieldSample.lastActivity = clockTime;
    assert(withShield.Poll(false).type == 4);
    shieldSample.connected = false; assert(withShield.Poll(false).type == 3);
    shieldSample.connected = true;
    const unsigned beforeReset = shieldResets;
    assert(withShield.Poll(true).type == 0 && shieldResets == beforeReset + 1);
    assert(withShield.Poll(false).type == 0);
    clockTime = 6000; shieldSample.lastActivity = clockTime;
    assert(withShield.Poll(false).type == 4);
    clockTime = 36000; assert(withShield.Poll(false).type == 0);
    shieldSample = {}; sony = {}; sony4 = {};

    // Backbone PS identity and selection across every existing controller family.
    for(int other : {1,2,3,4}) {
        ControllerSelector bb;
        pads={};connectedPads.fill(false);sony={};sony4={};shieldSample={};backboneSample={};
        clockTime=7000;backboneSample.connected=true;backboneSample.lastActivity=clockTime;
        backboneSample.gamepad.wButtons=XINPUT_GAMEPAD_X;
        assert(bb.Poll(false).type==5 && bb.Poll(false).gamepad.wButtons==XINPUT_GAMEPAD_X);
        clockTime=7010;
        if(other==1){connectedPads[0]=true;pads[0].Gamepad.wButtons=XINPUT_GAMEPAD_A;}
        if(other==2){sony.connected=true;sony.lastActivity=clockTime;}
        if(other==3){sony4.connected=true;sony4.lastActivity=clockTime;}
        if(other==4){shieldSample.connected=true;shieldSample.lastActivity=clockTime;}
        assert(bb.Poll(false).type==other);
        pads={};clockTime=7011;assert(bb.Poll(false).type==other);
        clockTime=7020;backboneSample.lastActivity=clockTime;
        assert(bb.Poll(false).type==5);
        backboneSample.connected=false;assert(bb.Poll(false).type==other);
        backboneSample.connected=true;clockTime=7030;backboneSample.lastActivity=clockTime;
        assert(bb.Poll(false).type==5);
        const auto resetsBefore=backboneResets;
        assert(bb.Poll(true).type==0 && backboneResets==resetsBefore+1);
        assert(bb.Poll(false).type==0);
        clockTime=8000;backboneSample.lastActivity=clockTime;assert(bb.Poll(false).type==5);
        clockTime=38000;assert(bb.Poll(false).type==0);
    }
    backboneSample={};sony={};sony4={};shieldSample={};connectedPads.fill(false);

    // VX-2 PS identity and selection across every existing controller family.
    for(int other : {1,2,3,4,5}) {
        ControllerSelector bb;
        pads={};connectedPads.fill(false);sony={};sony4={};shieldSample={};backboneSample={};vx2Sample={};
        clockTime=7000;vx2Sample.connected=true;vx2Sample.lastActivity=clockTime;
        vx2Sample.gamepad.wButtons=XINPUT_GAMEPAD_X;
        assert(bb.Poll(false).type==6 && bb.Poll(false).gamepad.wButtons==XINPUT_GAMEPAD_X);
        clockTime=7010;
        if(other==1){connectedPads[0]=true;pads[0].Gamepad.wButtons=XINPUT_GAMEPAD_A;}
        if(other==2){sony.connected=true;sony.lastActivity=clockTime;}
        if(other==3){sony4.connected=true;sony4.lastActivity=clockTime;}
        if(other==4){shieldSample.connected=true;shieldSample.lastActivity=clockTime;}
        if(other==5){backboneSample.connected=true;backboneSample.lastActivity=clockTime;}
        assert(bb.Poll(false).type==other);
        pads={};clockTime=7011;assert(bb.Poll(false).type==other);
        clockTime=7020;vx2Sample.lastActivity=clockTime;
        assert(bb.Poll(false).type==6);
        vx2Sample.connected=false;assert(bb.Poll(false).type==other);
        vx2Sample.connected=true;clockTime=7030;vx2Sample.lastActivity=clockTime;
        assert(bb.Poll(false).type==6);
        const auto resetsBefore=vx2Resets;
        assert(bb.Poll(true).type==0 && vx2Resets==resetsBefore+1);
        assert(bb.Poll(false).type==0);
        clockTime=8000;vx2Sample.lastActivity=clockTime;assert(bb.Poll(false).type==6);
        clockTime=38000;assert(bb.Poll(false).type==0);
    }
    vx2Sample={};backboneSample={};sony={};sony4={};shieldSample={};connectedPads.fill(false);

    // Paired Joy-Cons participate in the same activity arbitration as all families.
    for (int other : {1,2,3,4,5,6}) {
        ControllerSelector joySelection;
        pads={}; connectedPads.fill(false); sony={}; sony4={}; shieldSample={};
        backboneSample={}; vx2Sample={}; joyconSample={};
        clockTime=50000; joyconSample.connected=true; joyconSample.lastActivity=clockTime;
        joyconSample.gamepad.wButtons=XINPUT_GAMEPAD_B;
        assert(joySelection.Poll(false).type==7 && joySelection.Poll(false).gamepad.wButtons==XINPUT_GAMEPAD_B);
        clockTime=50010;
        if(other==1){connectedPads[0]=true;pads[0].Gamepad.wButtons=XINPUT_GAMEPAD_A;}
        if(other==2){sony.connected=true;sony.lastActivity=clockTime;}
        if(other==3){sony4.connected=true;sony4.lastActivity=clockTime;}
        if(other==4){shieldSample.connected=true;shieldSample.lastActivity=clockTime;}
        if(other==5){backboneSample.connected=true;backboneSample.lastActivity=clockTime;}
        if(other==6){vx2Sample.connected=true;vx2Sample.lastActivity=clockTime;}
        assert(joySelection.Poll(false).type==other);
        pads={}; clockTime=50011; assert(joySelection.Poll(false).type==other);
        clockTime=50020; joyconSample.lastActivity=clockTime;
        assert(joySelection.Poll(false).type==7);
        joyconSample.connected=false; assert(joySelection.Poll(false).type==other);
        joyconSample.connected=true;
        const unsigned before=joyconResets;
        assert(joySelection.Poll(true).type==0 && joyconResets==before+1);
        assert(joySelection.Poll(false).type==0);
        clockTime=51000; joyconSample.lastActivity=clockTime; assert(joySelection.Poll(false).type==7);
        clockTime=81000; assert(joySelection.Poll(false).type==0);
    }
    joyconSample={}; vx2Sample={}; backboneSample={}; sony={}; sony4={}; shieldSample={}; connectedPads.fill(false);

    // Existing wrap-safe activity timeout remains valid across GetTickCount wrap.
    ControllerSelector wrapped;
    connectedPads[1] = true;
    pads = {}; pads[1].Gamepad.wButtons = XINPUT_GAMEPAD_A;
    clockTime = 0xFFFFFFF0u;
    assert(wrapped.Poll(false).type == 1);
    pads[1] = {}; clockTime = 20;
    assert(wrapped.Poll(false).type == 1);
    clockTime = 40000;
    assert(wrapped.Poll(false).type == 0);
    return 0;
}

