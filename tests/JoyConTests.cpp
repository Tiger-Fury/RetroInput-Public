#include "../RetroInput/Core/JoyConReport.h"
#include <cassert>
#include <cstring>
using namespace RetroInput::JoyConReports;
static void Pack(BYTE* p,unsigned x,unsigned y) {p[0]=BYTE(x);p[1]=BYTE((x>>8)|((y&15)<<4));p[2]=BYTE(y>>4);}
int main() {
 assert(IsSupportedDevice(0x057E,0x2006) && IsSupportedDevice(0x057E,0x2007));
 assert(!IsSupportedDevice(0x057E,0x2009) && !IsSupportedDevice(0x054C,0x2006));
 BYTE data[9]={};Pack(data,1000,1100);Pack(data+3,2000,2100);Pack(data+6,900,1000);
 Calibration left,right;
 assert(ReadCalibration(data,9,false,left));
 assert(left.x.center==2000 && left.y.center==2100 && left.x.above==1000 && left.y.below==1000);
 Pack(data,2000,2100);Pack(data+3,900,1000);Pack(data+6,1000,1100);
 assert(ReadCalibration(data,9,true,right));
 assert(!ReadCalibration(data,8,true,right));
 memset(data,0xFF,sizeof(data));assert(!ReadCalibration(data,9,true,right));
 memset(data,0,sizeof(data));assert(!ReadCalibration(data,9,true,right));
 BYTE report[49]={0x30};Pack(report+6,2000,2100);Pack(report+9,2000,2100);
 XINPUT_GAMEPAD pad={};assert(Decode(report,sizeof(report),false,left,pad));
 assert(!pad.wButtons && !pad.sThumbLX && !pad.sThumbLY);
 const WORD rightButtons[]={XINPUT_GAMEPAD_X,XINPUT_GAMEPAD_Y,XINPUT_GAMEPAD_A,XINPUT_GAMEPAD_B};
 for(unsigned i=0;i<4;++i){report[3]=BYTE(1<<i);assert(Decode(report,49,true,right,pad));assert(pad.wButtons==rightButtons[i]);}
 report[3]=0xC0; report[4]=6;assert(Decode(report,49,true,right,pad));
 assert(pad.wButtons==(XINPUT_GAMEPAD_START|XINPUT_GAMEPAD_RIGHT_THUMB|XINPUT_GAMEPAD_RIGHT_SHOULDER) && pad.bRightTrigger==255 && !pad.bLeftTrigger);
 report[3]=0; report[4]=9; report[5]=0xCF;assert(Decode(report,49,false,left,pad));
 assert(pad.wButtons==(XINPUT_GAMEPAD_DPAD_UP|XINPUT_GAMEPAD_DPAD_DOWN|XINPUT_GAMEPAD_DPAD_LEFT|XINPUT_GAMEPAD_DPAD_RIGHT|XINPUT_GAMEPAD_LEFT_SHOULDER|XINPUT_GAMEPAD_BACK|XINPUT_GAMEPAD_LEFT_THUMB));
 assert(pad.bLeftTrigger==255 && !pad.bRightTrigger);
 Pack(report+6,3000,1100);assert(Decode(report,49,false,left,pad));assert(pad.sThumbLX==32767 && pad.sThumbLY==-32768);
 Pack(report+9,0,4095);assert(Decode(report,49,true,right,pad));assert(pad.sThumbRX==-32768 && pad.sThumbRY==32767);
 assert(!Decode(report,12,true,right,pad));assert(!pad.wButtons && !pad.sThumbRX);
 report[0]=0x21;assert(!Decode(report,49,true,right,pad));assert(!Decode(nullptr,49,true,right,pad));
 Half a,b;DWORD activity=123;
 a.connected=b.connected=true;a.received=b.received=100;a.activity=90;b.activity=95;
 a.pad.wButtons=XINPUT_GAMEPAD_DPAD_UP;b.pad.wButtons=XINPUT_GAMEPAD_A;
 a.pad.sThumbLX=2222;b.pad.sThumbRY=-4444;a.pad.bLeftTrigger=255;b.pad.bRightTrigger=255;
 assert(Combine(a,b,101,pad,activity));assert(activity==95 && pad.wButtons==(XINPUT_GAMEPAD_DPAD_UP|XINPUT_GAMEPAD_A));
 assert(pad.sThumbLX==2222 && pad.sThumbRY==-4444 && pad.bLeftTrigger==255 && pad.bRightTrigger==255);
 b.connected=false;assert(!Combine(a,b,102,pad,activity));assert(!pad.wButtons && !pad.sThumbLX && !activity);
 b.connected=true;assert(!Combine(a,b,1100,pad,activity));
 a.received=b.received=0xFFFFFFF0u;a.activity=0xFFFFFFE0u;b.activity=5;
 assert(Combine(a,b,10,pad,activity) && activity==5);
}
