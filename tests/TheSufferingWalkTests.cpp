#include "../RetroInput/Games/TheSuffering/InputMapping.h"
#include "../RetroInput/Games/TheSufferingTTB/InputMapping.h"
#include "../RetroInput/Core/XInputAggregator.h"
#include <array>
#include <cassert>
int main() {
 RetroInput::TheSuffering::InputMapping original;
 RetroInput::TheSufferingTTB::InputMapping ttb;
 std::array<float,256> a={},b={};XINPUT_GAMEPAD pad={};DWORD now=1000;
 auto sample=[&](SHORT x,SHORT y,bool shift,bool up,bool right) {
  a.fill(0);b.fill(0);pad={};pad.sThumbLX=x;pad.sThumbLY=y;
  original.Apply(0x005B5418,a.data(),pad,now);
  ttb.Apply(RetroInput::TheSufferingTTB::KeyboardRecord,b.data(),pad,now);
  assert(bool(a[0x2A])==shift && bool(a[0xC8])==up && bool(a[0xCD])==right);
  for(unsigned scan:{0x2Au,0xC8u,0xD0u,0xCBu,0xCDu})assert(a[scan]==b[scan]);
 };
 sample(0,0,false,false,false);sample(4915,0,false,false,false);
 sample(5000,0,true,false,true);sample(15000,0,true,false,true);
 sample(16000,0,false,false,true);sample(14500,0,false,false,true);
 sample(13600,0,true,false,true);sample(0,0,true,false,false);
 now+=249;sample(1000,0,true,false,false);++now;sample(0,0,false,false,false);
 sample(4000,4000,true,true,true);sample(0,0,true,false,false);
 now+=20;sample(32767,0,false,false,true);sample(0,0,false,false,false);
 sample(6000,0,true,false,true);original.Reset();ttb.Reset();sample(0,0,false,false,false);
 now=0xFFFFFFF0u;sample(6000,0,true,false,true);sample(0,0,true,false,false);
 now+=249;sample(0,0,true,false,false);++now;sample(0,0,false,false,false);
 a.fill(0);a[0x2A]=0.75f;pad={};pad.sThumbLX=32767;
 original.Apply(0x005B5418,a.data(),pad,now);assert(a[0x2A]==0.75f);
 std::array<float,14> mouse={};pad={};pad.bRightTrigger=255;pad.sThumbRX=32767;
 original.Apply(0x005B5440,mouse.data(),pad,now);assert(mouse[6]==1 && mouse[3]==0.75f);
 RetroController::XInputAggregator merger;
 std::array<XINPUT_STATE,XUSER_MAX_COUNT> states={};std::array<bool,XUSER_MAX_COUNT> connected={true,true,false,false};
 states[0].Gamepad.sThumbLX=5000;
 assert(merger.Update(states,connected).gamepad.sThumbLX==5000);
 assert(RetroInput::Controllers::HasActivity(states[0].Gamepad));
 states[1].Gamepad.sThumbLX=4000;states[1].Gamepad.sThumbLY=4000;
 assert(merger.Update(states,connected).gamepad.sThumbLY==4000);
 connected[1]=false;assert(merger.Update(states,connected).gamepad.sThumbLX==5000);
 states[0].Gamepad.sThumbLX=1000;assert(!RetroInput::Controllers::HasActivity(merger.Update(states,connected).gamepad));
}
