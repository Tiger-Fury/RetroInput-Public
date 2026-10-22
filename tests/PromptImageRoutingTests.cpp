#include "../RetroInput/Games/TheSuffering/ButtonRenderer.h"
#include <cassert>
std::atomic<int> g_controllerType{0};
static RetroInput::Buttons::Controller requested;
static RetroInput::Buttons::Button button;
namespace RetroInput::Buttons {
ButtonImage GetButtonImage(Controller controller, Button control) {
    requested = controller; button = control; return {};
}
}
int main() {
    using C = RetroInput::Buttons::Controller;
    using B = RetroInput::Buttons::Button;
    using RetroInput::TheSuffering::ButtonRenderer::GetPromptButtonImage;
    // Retain each cached family code while changing active artwork in both
    // directions, including the differently ordered Square/X and Circle/B.
    const B expected[] = {
        B::South,B::West,B::East,B::North,B::South,B::East,B::West,B::North,
        B::LeftShoulder,B::RightShoulder,B::LeftTrigger,B::RightTrigger,
        B::LeftShoulder,B::RightShoulder,B::LeftTrigger,B::RightTrigger,
        B::DpadUp,B::DpadDown,B::DpadLeft,B::DpadRight,B::DpadLeftRight,
        B::Menu,B::View,B::Menu,B::View,B::LeftStick,B::RightStick
    };
    for (unsigned kind = 0; kind < 27; ++kind) {
        for (C active : {C::Xbox,C::DualShock4,C::PlayStation,C::Nintendo,C::Xbox}) {
            GetPromptButtonImage(kind,active);
            assert(requested == active && button == expected[kind]);
        }
    }
    requested = C::Xbox;
    assert(!GetPromptButtonImage(27,C::DualShock4));
    assert(requested == C::Xbox); // Invalid codes do not issue a request.
}

