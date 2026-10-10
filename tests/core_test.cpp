#include "../DenDeco/Core.h"
#include <cassert>
#include <iostream>
#include <vector>
using namespace Deco;
int main(){
 Controller c;assert(!c.settings.power);c.apply({Action::Power,1},0);
 c.apply({Action::Color,0,10,20,30},1);c.apply({Action::Brightness,75},2);c.apply({Action::Power,0},3);
 assert(c.settings.light.brightness==75&&c.settings.light.r==10);assert(!c.apply({Action::TimerStart,30},4).accepted);
 c.apply({Action::SleepNow},5);assert(c.settings.power&&c.settings.light.brightness==5);
 c.apply({Action::Brightness,12},6);c.apply({Action::Mode,Solid},7);c.apply({Action::Mode,Sleep},8);assert(c.settings.light.brightness==12);
 c.apply({Action::TimerStart,30},100);c.apply({Action::Brightness,12},200);assert(c.timer.active);
 c.apply({Action::FavoriteSave,0},201);assert(c.timer.active);c.apply({Action::Brightness,13},202);assert(!c.timer.active);
 c.apply({Action::Power,0},203);c.apply({Action::FavoriteApply,0},204);assert(!c.settings.power&&c.settings.light.brightness==12);
 assert(!c.apply({Action::Brightness,0},205).accepted);assert(!c.apply({Action::Mode,99},205).accepted);assert(!c.apply({Action::Cap,31},205).accepted);
 c.apply({Action::Power,1},206);c.apply({Action::TimerStart,1},0xffffff00u);assert(c.timer.remaining(0x100u)==59488u);assert(c.tick(0xffffff00u+60000u).changed&&!c.settings.power);
 Timer t;t.begin(0,1);assert(t.envelope(30000)==.5f&&t.remaining(60000)==0);float previous=0;for(int p=1;p<=100;p++){float v=scale(p,30);assert(v>=previous&&v<=.300001f);previous=v;}
 std::vector<TouchEvent> events;Touch touch;bool raw[4]={};auto scan=[&](uint32_t n){touch.scan(raw,n,[&](TouchEvent e){events.push_back(e);});};touch.begin(0);scan(1000);scan(1100);assert(events.empty());
 raw[0]=true;scan(1200);scan(1230);raw[0]=false;scan(1300);scan(1330);scan(1400);assert(events.size()==1&&events[0]==TouchEvent::PowerShort);
 events.clear();raw[0]=true;scan(1500);scan(1530);scan(3530);scan(4000);raw[0]=false;scan(4100);scan(4130);assert(events.size()==1&&events[0]==TouchEvent::SleepHold);
 events.clear();raw[2]=true;scan(4200);scan(4230);scan(4830);scan(4880);raw[2]=false;scan(4900);scan(4930);assert(events.size()==2&&events[0]==TouchEvent::PlusStep);
 events.clear();raw[0]=raw[1]=true;scan(5000);scan(5030);scan(8000);raw[0]=raw[1]=false;scan(8100);scan(8130);assert(events.empty());
 Touch bootHeld;raw[0]=true;bootHeld.begin(0);bootHeld.scan(raw,10,[&](TouchEvent e){events.push_back(e);});bootHeld.scan(raw,4000,[&](TouchEvent e){events.push_back(e);});raw[0]=false;bootHeld.scan(raw,5000,[&](TouchEvent e){events.push_back(e);});bootHeld.scan(raw,5030,[&](TouchEvent e){events.push_back(e);});assert(events.empty());
 Renderer render;Settings s;s.power=true;s.light.mode=Solid;s.light.r=s.light.g=s.light.b=255;s.light.brightness=100;Timer noTimer;uint8_t frame[Config::LED_COUNT][3];render.begin(s,0);render.frame(s,noTimer,800,frame);assert(frame[0][0]==76);
 s.power=false;render.retarget(s,800);render.frame(s,noTimer,1600,frame);for(auto& row:frame)for(auto x:row)assert(x==0);
 s.power=true;s.light.mode=Rise;render.begin(s,0);for(uint32_t n=800;n<20000;n+=33){render.frame(s,noTimer,n,frame);for(int i=0;i<10;i++)for(int ch=0;ch<3;ch++)assert(frame[i][ch]==frame[19-i][ch]);}
 s.light.mode=Sleep;s.light.brightness=5;render.begin(s,0);render.frame(s,noTimer,800,frame);assert(frame[0][0]>frame[0][1]&&frame[0][1]>0&&frame[0][2]>0);
 // Cancelling a timer in its fade must start from the displayed brightness.
 s.light.mode=Solid;s.light.brightness=100;s.light.r=s.light.g=s.light.b=255;render.begin(s,0);t.begin(0,1);render.frame(s,t,30000,frame);int faded=frame[0][0];t.cancel();render.retarget(s,30000);render.frame(s,t,30000,frame);assert(frame[0][0]==faded);render.frame(s,t,30800,frame);assert(frame[0][0]==76);
 s.light.mode=Candle;render.begin(s,0);for(uint32_t n=800;n<60000;n+=33){render.frame(s,noTimer,n,frame);for(auto& row:frame)for(auto x:row)assert(x<=76);}
 std::cout<<"PASS: controller, sleep memory, timer rollover, touch/holds/multi-touch, gamma, cap, fade and paired LED mapping\n";
}
