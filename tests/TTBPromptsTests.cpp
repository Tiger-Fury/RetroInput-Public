#include "../RetroInput/Games/TheSufferingTTB/ButtonRenderer.h"
#include "../RetroInput/Games/TheSufferingTTB/Prompts.h"
#include <cassert>
std::atomic<bool> g_controllerActive{true};
std::atomic<int> g_controllerType{2};
static bool imagesEnabled=true;
extern "C" BOOL AreInlineGlyphsEnabled() {return imagesEnabled;}
static RetroInput::Buttons::Controller requestedController;
static RetroInput::Buttons::Button requestedButton;
static bool missingImage=false;
static const std::uint32_t pixel=0;
static const RetroInput::Buttons::AtlasImage atlas{&pixel,1024,1024,4096,RetroInput::Buttons::PixelFormat::StraightAlphaARGB32};
namespace RetroInput::Buttons {
ButtonImage GetButtonImage(Controller controller,Button button) {
    requestedController=controller;requestedButton=button;
    return missingImage?ButtonImage{}:ButtonImage{&atlas,128,256,128,128};
}
ButtonImage GetButtonImage(IconPack,Button) {return {};}
}
namespace Render=RetroInput::TheSufferingTTB::ButtonRenderer;
namespace Text=RetroInput::TheSufferingTTB::PromptText;
namespace Prompts=RetroInput::TheSufferingTTB::Prompts;
static std::wstring expanded;
static wchar_t* __fastcall FakeExpand(void* object,void*,const wchar_t* text,int context) {
    assert(object==reinterpret_cast<void*>(123) && context==42);
    expanded=text;return expanded.data();
}
static Render::GlyphRecord normal{0, L'X',3,7,2,1,10,0};
static Render::GlyphRecord* __fastcall FakeLookup(void*,void*,const wchar_t*) {return &normal;}
static int drawnSource,drawnWidth,drawnMode,drawnY;
static unsigned drawnFlags;
static std::uint32_t drawnColour;
static void* drawnTexture;
static int __fastcall FakeDraw(void* texture,void*,int,int y,int source,int,int width,int,
    const void*,unsigned flags,const std::uint32_t* colours,float,float,int mode) {
    drawnTexture=texture;drawnSource=source;drawnWidth=width;drawnMode=mode;drawnY=y;drawnFlags=flags;
    drawnColour=colours?colours[0]:0;return 92;
}
int main() {
    using B=RetroInput::Buttons::Button;using C=RetroInput::Buttons::Controller;
    assert(Text::Transform(L"Press <PressAndHold> to pick up:",2,false)==L"Hold Triangle to pick up:");
    assert(Text::Transform(L"Press <PressAndHold> to pick up:",7,false)==L"Hold X to pick up:");
    assert(Text::Transform(L"<GUISelect> <GUIDelete> <GUICancel>",1,false)==L"X <GUIDelete> Start");
    assert(Text::Transform(L"<Struggle> <Crouch> <Flashlight>",2,false)==L"R2 L3 R3");
    assert(Text::Transform(L"<PrevScreen> <ScrollLeft> <NextScreen> <ScrollRight>",1,false)==L"D-pad Left D-pad Left D-pad Right D-pad Right");
    assert(Text::Transform(L"<Unknown> <GUIDelete>",2,true)==L"<Unknown> <GUIDelete>");
    assert(Text::Transform(L"Press <PressAndHold>",0,true)==L"Press <PressAndHold>");
    const wchar_t* tokens[]={L"<Attack>",L"<Struggle>",L"<Throw>",L"<SecondaryAttack>",L"<Jump>",L"<Use>",L"<GUISelect>",L"<Inventory>",L"<PressAndHold>",L"<Crouch>",L"<Flashlight>",L"<Insanity>",L"<FirstPerson>",L"<Reload>",L"<Map>",L"<Move>",L"<LadderMove>",L"<Look>",L"<PrevScreen>",L"<ScrollLeft>",L"<NextScreen>",L"<ScrollRight>",L"<GUICancel>"};
    const B expected[]={B::RightTrigger,B::RightTrigger,B::LeftTrigger,B::East,B::South,B::West,B::West,B::North,B::North,B::LeftStickClick,B::RightStickClick,B::LeftShoulder,B::DpadUp,B::DpadDown,B::View,B::LeftStick,B::LeftStick,B::RightStick,B::DpadLeft,B::DpadLeft,B::DpadRight,B::DpadRight,B::Menu};
    for(unsigned i=0;i<23;++i)for(int device:{1,2,3,4,5,6,7}) {
        const auto text=Text::Transform(tokens[i],device,true);
        assert(text.size()==1 && text[0]==0xE100+static_cast<unsigned>(expected[i]));
    }
    Prompts::originalExpand=reinterpret_cast<Prompts::Expand>(&FakeExpand);
    Prompts::ExpandHook(reinterpret_cast<void*>(123),nullptr,L"<Jump> <GUIDelete>",42);
    assert(expanded==Text::Transform(L"<Jump> <GUIDelete>",2,true));
    g_controllerActive=false;
    Prompts::ExpandHook(reinterpret_cast<void*>(123),nullptr,L"<Jump> <GUIDelete>",42);
    assert(expanded==L"<Jump> <GUIDelete>");g_controllerActive=true;
    Render::originalLookup=reinterpret_cast<Render::Lookup>(&FakeLookup);
    const wchar_t character=static_cast<wchar_t>(0xE100+static_cast<unsigned>(B::North));
    auto glyph=Render::LookupHook(reinterpret_cast<void*>(111),nullptr,&character);
    assert(glyph!=&normal && glyph->width==15 && glyph->character==character && glyph->source>=Render::MarkerBase);
    assert(normal.width==10 && normal.source==7 && normal.left==2);
    Render::originalDraw=reinterpret_cast<Render::Draw>(&FakeDraw);
    Render::atlasTextures[0]={&atlas,reinterpret_cast<void*>(222),false};
    InterlockedExchange(&Render::inlineReady,1);
    const std::uint32_t colours[]={0x11223344u,0x22334455u,0x33445566u,0x44556677u};
    for(int device:{1,2,3,4,5,6,7}) {
        g_controllerType=device;
        const int result=Render::RenderFontGlyph(reinterpret_cast<void*>(333),10,20,glyph->source,0,15,12,nullptr,0x407,colours,2,2,9);
        assert(result==92 && requestedButton==B::North);
        const C controller=device==7?C::Nintendo:(device==3||device==6)?C::DualShock4:(device==2||device==5)?C::PlayStation:C::Xbox;
        assert(requestedController==controller && drawnTexture==reinterpret_cast<void*>(222));
        assert(drawnSource==128 && drawnWidth==128 && drawnMode==9 && drawnFlags==4 && drawnColour==0x11FFFFFFu && drawnY==13);
    }
    missingImage=true;
    Render::RenderFontGlyph(reinterpret_cast<void*>(333),10,20,glyph->source,0,15,12,nullptr,0x407,colours,2,2,9);
    assert(drawnTexture==reinterpret_cast<void*>(333) && drawnSource==7 && drawnWidth==10 && drawnFlags==0x407);
    assert(Render::DrawHook(reinterpret_cast<void*>(333),nullptr,10,20,4,5,6,7,nullptr,0x407,colours,2,3,8)==92);
    assert(drawnSource==4 && drawnMode==8 && drawnFlags==0x407);
}
