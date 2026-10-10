#include "../DenDeco/Core.h"
#include <cassert>
#include <iostream>
#include <vector>
using namespace Deco;
struct TouchFixture {
 Touch touch;bool raw[4]={};std::vector<TouchEvent> events;
 explicit TouchFixture(uint32_t boot=0){touch.begin(boot);scan(boot+Config::TOUCH_STARTUP_MS);}
 void scan(uint32_t now){touch.scan(raw,now,[&](TouchEvent e){events.push_back(e);});}
 void set(uint8_t mask,uint32_t now){for(int i=0;i<4;i++)raw[i]=(mask&(1u<<i))!=0;scan(now);}
 void stable(uint8_t mask,uint32_t now){set(mask,now);scan(now+Config::DEBOUNCE_MS);}
};
static void testWiFiResetChord(){
 constexpr uint8_t minusPlus=6;
 // The timer starts at the second button's debounced press, not the first.
 TouchFixture success;success.stable(2,1200);success.set(minusPlus,1300);
 success.scan(1329);success.scan(1330);
 success.scan(1330+Config::WIFI_RESET_HOLD_MS-1);assert(success.events.empty());
 success.scan(1330+Config::WIFI_RESET_HOLD_MS);
 assert(success.events.size()==1&&success.events[0]==TouchEvent::WiFiResetHold);
 success.scan(10000);success.scan(15000);assert(success.events.size()==1);
 success.stable(4,15010);success.scan(18000);assert(success.events.size()==1);
 success.stable(0,18010);success.scan(19000);assert(success.events.size()==1);
 // A fully released chord rearms; no short/repeat is emitted when it ends.
 success.stable(minusPlus,20000);success.scan(20030+Config::WIFI_RESET_HOLD_MS);
 assert(success.events.size()==2&&success.events[1]==TouchEvent::WiFiResetHold);
 success.stable(0,26000);success.stable(4,27000);success.stable(0,27100);
 assert(success.events.size()==3&&success.events[2]==TouchEvent::PlusShort);

 // A partial release just before the deadline cancels immediately, including
 // during release debounce. Repressing it must not rearm until all release.
 TouchFixture cancelled;cancelled.stable(minusPlus,1200);
 cancelled.set(4,1230+Config::WIFI_RESET_HOLD_MS-1);
 cancelled.scan(1230+Config::WIFI_RESET_HOLD_MS);assert(cancelled.events.empty());
 cancelled.stable(minusPlus,6300);cancelled.scan(12000);assert(cancelled.events.empty());
 cancelled.stable(2,12100);cancelled.scan(18000);assert(cancelled.events.empty());
 cancelled.stable(0,18100);cancelled.stable(minusPlus,19000);
 cancelled.scan(19030+Config::WIFI_RESET_HOLD_MS);
 assert(cancelled.events.size()==1&&cancelled.events[0]==TouchEvent::WiFiResetHold);

 // An extra key cancels even when it has not debounced at the deadline.
 TouchFixture triple;triple.stable(minusPlus,1200);
 triple.set(7,1230+Config::WIFI_RESET_HOLD_MS);triple.scan(7000);
 triple.stable(minusPlus,7100);triple.scan(13000);assert(triple.events.empty());
 triple.stable(0,13100);triple.stable(minusPlus,14000);
 triple.scan(14030+Config::WIFI_RESET_HOLD_MS);
 assert(triple.events.size()==1&&triple.events[0]==TouchEvent::WiFiResetHold);

 // Every other multi-button combination remains cancelled, including
 // transitioning from that combination directly into + and -.
 for(uint8_t mask=1;mask<16;mask++){
  if(mask==minusPlus||(mask&(mask-1))==0)continue;
  TouchFixture other;other.stable(mask,1200);other.scan(8000);
  other.stable(minusPlus,8100);other.scan(15000);assert(other.events.empty());
  other.stable(0,15100);other.stable(1,16000);other.stable(0,16100);
  assert(other.events.size()==1&&other.events[0]==TouchEvent::PowerShort);
 }

 // A candidate abandoned before the second press debounces cannot turn
 // into a single-key brightness hold or a new chord without full release.
 TouchFixture debounce;debounce.stable(2,1200);debounce.set(minusPlus,1300);
 debounce.set(2,1310);debounce.scan(5000);debounce.stable(minusPlus,5100);
 debounce.scan(12000);assert(debounce.events.empty());
 debounce.stable(0,12100);debounce.stable(2,13000);debounce.stable(0,13100);
 assert(debounce.events.size()==1&&debounce.events[0]==TouchEvent::MinusShort);

 // Held-at-boot inputs do not trigger or arm the reset until all release.
 TouchFixture startup;startup.touch.begin(0);startup.set(minusPlus,10);
 startup.scan(40);startup.scan(8000);assert(startup.events.empty());
 startup.stable(4,8100);startup.stable(minusPlus,8200);startup.scan(14000);
 assert(startup.events.empty());startup.stable(0,14100);
 startup.stable(minusPlus,15000);startup.scan(15030+Config::WIFI_RESET_HOLD_MS);
 assert(startup.events.size()==1&&startup.events[0]==TouchEvent::WiFiResetHold);

 // millis() wraps while holding both buttons; elapsed comparisons stay valid.
 const uint32_t boot=0xffffef00u,pressed=boot+1200u;
 TouchFixture rollover(boot);rollover.stable(minusPlus,pressed);
 const uint32_t heldAt=pressed+Config::DEBOUNCE_MS;
 rollover.scan(heldAt+Config::WIFI_RESET_HOLD_MS-1);assert(rollover.events.empty());
 rollover.scan(heldAt+Config::WIFI_RESET_HOLD_MS);
 assert(rollover.events.size()==1&&rollover.events[0]==TouchEvent::WiFiResetHold);
 rollover.stable(0,heldAt+Config::WIFI_RESET_HOLD_MS+100);
 rollover.scan(heldAt+Config::WIFI_RESET_HOLD_MS+1000);assert(rollover.events.size()==1);
}
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
 testWiFiResetChord();
 Renderer render;Settings s;s.power=true;s.light.mode=Solid;s.light.r=s.light.g=s.light.b=255;s.light.brightness=100;Timer noTimer;uint8_t frame[Config::LED_COUNT][3];render.begin(s,0);render.frame(s,noTimer,800,frame);assert(frame[0][0]==76);
 s.power=false;render.retarget(s,800);render.frame(s,noTimer,1600,frame);for(auto& row:frame)for(auto x:row)assert(x==0);
 s.power=true;s.light.mode=Rise;render.begin(s,0);for(uint32_t n=800;n<20000;n+=33){render.frame(s,noTimer,n,frame);for(int i=0;i<10;i++)for(int ch=0;ch<3;ch++)assert(frame[i][ch]==frame[19-i][ch]);}
 s.light.mode=Sleep;s.light.brightness=5;render.begin(s,0);render.frame(s,noTimer,800,frame);assert(frame[0][0]>frame[0][1]&&frame[0][1]>0&&frame[0][2]>0);
 // Cancelling a timer in its fade must start from the displayed brightness.
 s.light.mode=Solid;s.light.brightness=100;s.light.r=s.light.g=s.light.b=255;render.begin(s,0);t.begin(0,1);render.frame(s,t,30000,frame);int faded=frame[0][0];t.cancel();render.retarget(s,30000);render.frame(s,t,30000,frame);assert(frame[0][0]==faded);render.frame(s,t,30800,frame);assert(frame[0][0]==76);
 s.light.mode=Candle;render.begin(s,0);for(uint32_t n=800;n<60000;n+=33){render.frame(s,noTimer,n,frame);for(auto& row:frame)for(auto x:row)assert(x<=76);}
 std::cout<<"PASS: controller, sleep memory, timer rollover, touch/holds/multi-touch, WiFi reset chord, gamma, cap, fade and paired LED mapping\n";
}
