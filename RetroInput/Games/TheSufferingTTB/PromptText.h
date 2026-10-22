#pragma once
#include <string>
#include "../../Core/ButtonImages.h"
#include "../../Core/ControllerTypes.h"
namespace RetroInput::TheSufferingTTB::PromptText {
using B = Buttons::Button;
inline void ReplaceAll(std::wstring& text, const wchar_t* find, const std::wstring& value) {
    const std::wstring needle(find);
    std::size_t position = 0;
    while ((position=text.find(needle,position)) != std::wstring::npos) {
        text.replace(position,needle.size(),value);position += value.size();
    }
}
inline const wchar_t* Name(B button,int device) {
    const bool ps=Controllers::IsPlayStation(device), nintendo=device==7;
    switch(button) {
    case B::South:return ps?L"Cross":nintendo?L"B":L"A";
    case B::East:return ps?L"Circle":nintendo?L"A":L"B";
    case B::West:return ps?L"Square":nintendo?L"Y":L"X";
    case B::North:return ps?L"Triangle":nintendo?L"X":L"Y";
    case B::LeftShoulder:return ps?L"L1":nintendo?L"L":L"LB";
    case B::RightShoulder:return ps?L"R1":nintendo?L"R":L"RB";
    case B::LeftTrigger:return ps?L"L2":nintendo?L"ZL":L"LT";
    case B::RightTrigger:return ps?L"R2":nintendo?L"ZR":L"RT";
    case B::Menu:return nintendo?L"Plus":ps&&device!=6?L"Options":L"Start";
    case B::View:return nintendo?L"Minus":device==3?L"Share":device==6?L"Select":ps?L"Create":L"View";
    case B::LeftStick:return L"Left Stick";
    case B::RightStick:return L"Right Stick";
    case B::LeftStickClick:return ps?L"L3":L"Left Stick Click";
    case B::RightStickClick:return ps?L"R3":L"Right Stick Click";
    case B::DpadUp:return L"D-pad Up";
    case B::DpadDown:return L"D-pad Down";
    case B::DpadLeft:return L"D-pad Left";
    case B::DpadRight:return L"D-pad Right";
    default:return L"?";
    }
}
inline std::wstring Transform(const wchar_t* text, int device, bool images) {
    if (!text) return {};
    std::wstring result(text);
    if (!device) return result;
    ReplaceAll(result,L"Press <PressAndHold>",L"Hold <PressAndHold>");
    const struct { const wchar_t* token; B button; } mappings[] = {
        {L"<Attack>",B::RightTrigger},{L"<Struggle>",B::RightTrigger},
        {L"<Throw>",B::LeftTrigger},{L"<SecondaryAttack>",B::East},
        {L"<Jump>",B::South},{L"<Use>",B::West},{L"<GUISelect>",B::West},
        {L"<Inventory>",B::North},{L"<PressAndHold>",B::North},
        {L"<Crouch>",B::LeftStickClick},{L"<Flashlight>",B::RightStickClick},
        {L"<Insanity>",B::LeftShoulder},{L"<FirstPerson>",B::DpadUp},
        {L"<Reload>",B::DpadDown},{L"<Map>",B::View},
        {L"<Move>",B::LeftStick},{L"<LadderMove>",B::LeftStick},{L"<Look>",B::RightStick},
        {L"<PrevScreen>",B::DpadLeft},{L"<ScrollLeft>",B::DpadLeft},
        {L"<NextScreen>",B::DpadRight},{L"<ScrollRight>",B::DpadRight},
        {L"<GUICancel>",B::Menu}
    };
    // Leave GUIDelete and unknown actions for the game's keyboard expansion.
    for (const auto& item:mappings) {
        const auto replacement=images ? std::wstring(1,static_cast<wchar_t>(0xE100+static_cast<unsigned>(item.button))) :
            std::wstring(Name(item.button,device));
        ReplaceAll(result,item.token,replacement);
    }
    return result;
}
} // namespace RetroInput::TheSufferingTTB::PromptText
