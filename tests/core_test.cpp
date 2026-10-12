#include "../DenDeco/Core.h"
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <vector>
using namespace Deco;
static_assert(Sleep==0&&Solid==1&&SlowColor==2&&Candle==3&&Rainbow==4&&Rise==5,"Persisted mode IDs changed");
static_assert(Breath==6&&Twinkle==7&&Aurora==8&&Ocean==9&&Forest==10&&Sunset==11&&Comet==12&&ColorWaves==13&&ModeCount==14,"New effect IDs changed");
static_assert(int(Action::Color)==3&&int(Action::TimerCancel)==10&&int(Action::Tint)==11,"Action IDs changed");
struct CapturedFrame {uint8_t pixels[Config::LED_COUNT][3]={};};
static bool sameFrame(const CapturedFrame& a,const CapturedFrame& b){
 for(int i=0;i<Config::LED_COUNT;i++)for(int ch=0;ch<3;ch++)if(a.pixels[i][ch]!=b.pixels[i][ch])return false;
 return true;
}
static unsigned frameTotal(const CapturedFrame& frame){
 unsigned total=0;for(auto& row:frame.pixels)for(auto value:row)total+=value;return total;
}
static unsigned channelTotal(const CapturedFrame& frame,int channel){
 unsigned total=0;for(auto& row:frame.pixels)total+=row[channel];return total;
}
static bool hasSpatialVariation(const CapturedFrame& frame){
 for(int i=1;i<Config::HEIGHT_COUNT;i++)for(int ch=0;ch<3;ch++)if(frame.pixels[i][ch]!=frame.pixels[0][ch])return true;
 return false;
}
static CapturedFrame capture(const Settings& settings,uint32_t elapsed,uint32_t start=0){
 Renderer renderer;Timer timer;CapturedFrame out;renderer.begin(settings,start);renderer.frame(settings,timer,start+elapsed,out.pixels);return out;
}
static void testNewEffectCommands(){
 Controller controller;controller.apply({Action::Power,1},0);
 for(int mode=0;mode<ModeCount;mode++)assert(controller.apply({Action::Mode,mode},1).accepted&&controller.settings.light.mode==mode);
 Settings previous=controller.settings;
 assert(!controller.apply({Action::Mode,ModeCount},2).accepted&&controller.settings==previous);
 for(int mode=Breath;mode<ModeCount;mode++){
  controller.apply({Action::Mode,mode},10);controller.apply({Action::Speed,73},11);controller.apply({Action::Brightness,64},12);
  controller.settings.light.r=26;controller.settings.light.g=171;controller.settings.light.b=211;
  Light saved=controller.settings.light;assert(validLight(saved));
  assert(controller.apply({Action::FavoriteSave,mode%3},13).accepted);
  controller.apply({Action::Color,0,1,2,3},14);controller.apply({Action::Speed,11},15);
  assert(controller.settings.light.mode==Solid);
  assert(controller.apply({Action::FavoriteApply,mode%3},16).accepted&&controller.settings.light==saved);
 }
 for(int mode=0;mode<ModeCount;mode++){
  controller.apply({Action::Mode,mode},20);controller.apply({Action::TimerStart,30},21);
  Settings before=controller.settings;uint32_t revision=controller.revision;
  Result tint=controller.apply({Action::Tint,0,77,88,99},22);
  if(mode==Solid||mode==Breath||mode==Twinkle||mode==Comet){
   assert(tint.accepted&&tint.changed&&controller.settings.light.mode==mode&&!controller.timer.active);
   assert(controller.settings.light.r==77&&controller.settings.light.g==88&&controller.settings.light.b==99);
   assert(controller.settings.light.brightness==before.light.brightness&&controller.settings.light.speed==before.light.speed&&controller.settings.power==before.power);
   controller.apply({Action::TimerStart,30},23);uint32_t unchangedRevision=controller.revision;
   Result noop=controller.apply({Action::Tint,0,77,88,99},24);
   assert(noop.accepted&&!noop.changed&&controller.timer.active&&controller.revision==unchangedRevision);
   controller.apply({Action::Tint,0,99,88,77},25);
  }else assert(!tint.accepted&&!tint.changed&&controller.settings==before&&controller.timer.active&&controller.revision==revision);
 }
 Light invalid;invalid.mode=ModeCount;assert(!validLight(invalid));
}
static void testNewEffectRendering(){
 Settings settings;settings.power=true;settings.light.brightness=100;settings.light.r=settings.light.g=settings.light.b=255;
 Timer noTimer;
 // Every effect obeys the shared cap, power fade and timer envelope. The
 // lowest allowed cap matters because rounding can exceed it without a clamp.
 for(int mode=0;mode<ModeCount;mode++)for(int speed:{1,50,100})for(int cap:{3,30}){
  settings.light.mode=mode;settings.light.speed=speed;settings.cap=cap;
  Renderer renderer;CapturedFrame frame;renderer.begin(settings,0);
  renderer.frame(settings,noTimer,0,frame.pixels);assert(frameTotal(frame)==0);
  const int ceiling=cap*255/100;
  for(uint32_t now=1000;now<18000;now+=97){
   renderer.frame(settings,noTimer,now,frame.pixels);
   for(auto& row:frame.pixels)for(auto value:row)assert(value<=ceiling);
   if(mode>=Breath)for(int i=0;i<Config::HEIGHT_COUNT;i++)for(int ch=0;ch<3;ch++)assert(frame.pixels[i][ch]==frame.pixels[Config::LED_COUNT-1-i][ch]);
  }
  renderer.frame(settings,noTimer,18000,frame.pixels);CapturedFrame visible=frame;
  settings.power=false;renderer.retarget(settings,18000);renderer.frame(settings,noTimer,18000,frame.pixels);assert(sameFrame(frame,visible));
  renderer.frame(settings,noTimer,18000+Config::FADE_MS,frame.pixels);assert(frameTotal(frame)==0);
  renderer.frame(settings,noTimer,25000,frame.pixels);assert(frameTotal(frame)==0);settings.power=true;
  Timer timer;timer.begin(0,1);renderer.begin(settings,0);renderer.frame(settings,timer,30000,frame.pixels);
  CapturedFrame full=capture(settings,30000);
  // The timer envelope scales the actual capped peak before either static
  // or spatial rounding, keeping half-intensity error within one code.
  for(int i=0;i<Config::LED_COUNT;i++)for(int ch=0;ch<3;ch++)assert(std::abs(int(frame.pixels[i][ch])*2-int(full.pixels[i][ch]))<=1);
 }
 settings.cap=30;
 for(int mode=Breath;mode<ModeCount;mode++)for(int speed:{1,50,100}){
  settings.light.mode=mode;settings.light.speed=speed;
  CapturedFrame first=capture(settings,1000);bool temporal=false,spatial=false;
  for(uint32_t now=1000;now<=31000;now+=613){
   CapturedFrame frame=capture(settings,now);
   assert(sameFrame(frame,capture(settings,now))); // Repeatability, no random state.
   assert(sameFrame(frame,capture(settings,now,0xfffff100u))); // millis rollover.
   temporal=temporal||!sameFrame(frame,first);spatial=spatial||hasSpatialVariation(frame);
   if(mode==Breath)assert(!hasSpatialVariation(frame));
  }
  assert(temporal);assert(mode==Breath||spatial);
  // Output after the fade is independent of skipped/intermediate frames.
  Renderer continuous;continuous.begin(settings,0);CapturedFrame previous,current;
  continuous.frame(settings,noTimer,1000,previous.pixels);
  for(uint32_t now=1033;now<65000;now+=33){
   continuous.frame(settings,noTimer,now,current.pixels);
   assert(sameFrame(current,capture(settings,now)));
   for(int i=0;i<Config::LED_COUNT;i++)for(int ch=0;ch<3;ch++)assert(std::abs(int(current.pixels[i][ch])-int(previous.pixels[i][ch]))<=10);
   previous=current;
  }
  // Long-running lights retain millisecond motion instead of quantizing
  // time to coarse float seconds after several weeks of uptime.
  previous=capture(settings,3000000000u);
  for(uint32_t now=3000000033u;now<3000003000u;now+=33){
   current=capture(settings,now);
   for(int i=0;i<Config::LED_COUNT;i++)for(int ch=0;ch<3;ch++)assert(std::abs(int(current.pixels[i][ch])-int(previous.pixels[i][ch]))<=10);
   previous=current;
  }
  Settings dim=settings;dim.light.brightness=10;
  assert(frameTotal(capture(dim,5100))<frameTotal(capture(settings,5100)));
 }
 // Speed must affect every new effect, and each must produce a distinct
 // spatial/time signature rather than relabeling the same animation.
 for(int mode=Breath;mode<ModeCount;mode++)for(int a:{1,50,100})for(int b:{1,50,100}){
  if(a>=b)continue;
  bool different=false;settings.light.mode=mode;
  for(uint32_t now:{1000u,5100u,10700u,23000u}){
   settings.light.speed=a;CapturedFrame slow=capture(settings,now);settings.light.speed=b;
   different=different||!sameFrame(slow,capture(settings,now));
  }
  assert(different);
 }
 for(int speed:{1,50,100})for(int a=Breath;a<ModeCount;a++)for(int b=a+1;b<ModeCount;b++){
  bool different=false;settings.light.speed=speed;
  for(uint32_t now:{1000u,7000u,13000u}){
   settings.light.mode=a;CapturedFrame one=capture(settings,now);settings.light.mode=b;
   different=different||!sameFrame(one,capture(settings,now));
  }
  assert(different);
 }
 // Tint colors only the three promised effects. Fixed palettes remain
 // unchanged when a remembered RGB selection is different.
 settings.light.speed=50;
 for(int mode=Breath;mode<ModeCount;mode++){
  settings.light.mode=mode;settings.light.r=255;settings.light.g=0;settings.light.b=0;
  CapturedFrame red=capture(settings,11000);
  settings.light.r=0;settings.light.g=0;settings.light.b=255;CapturedFrame blue=capture(settings,11000);
  if(mode==Breath||mode==Twinkle||mode==Comet){
   assert(!sameFrame(red,blue));
   for(int i=0;i<Config::LED_COUNT;i++){assert(red.pixels[i][1]==0&&red.pixels[i][2]==0);assert(blue.pixels[i][0]==0&&blue.pixels[i][1]==0);}
  }else assert(sameFrame(red,blue));
  settings.light.mode=Solid;settings.light.r=settings.light.g=settings.light.b=255;
  Renderer transition;transition.begin(settings,0);CapturedFrame before,after;transition.frame(settings,noTimer,1000,before.pixels);
  settings.light.mode=mode;transition.retarget(settings,1000);transition.frame(settings,noTimer,1000,after.pixels);
  assert(sameFrame(before,after));transition.frame(settings,noTimer,1000+Config::FADE_MS,after.pixels);
  assert(sameFrame(after,capture(settings,Config::FADE_MS)));
 }
}
static void testColorQuantization(){
 Settings settings;settings.power=true;settings.light.mode=SlowColor;settings.light.brightness=5;
 Timer timer;CapturedFrame previous,current;unsigned changes=0,maxHold=0;
 for(int speed:{1,50,100}){
  settings.light.speed=speed;Renderer renderer;renderer.begin(settings,0);renderer.frame(settings,timer,1000,previous.pixels);
  unsigned hold=0;changes=0;maxHold=0;
  for(uint32_t now=1016;now<121000;now+=16){
   renderer.frame(settings,timer,now,current.pixels);
   for(int channel=0;channel<3;channel++){
    // A paired-height change is 2/20 = 0.1 code of aggregate light,
    // instead of changing all 20 LEDs by one full code together.
    assert(std::abs(int(channelTotal(current,channel))-int(channelTotal(previous,channel)))<=2);
    uint8_t low=255,high=0;
    for(auto& row:current.pixels){if(row[channel]<low)low=row[channel];if(row[channel]>high)high=row[channel];}
    assert(high-low<=1);
    for(int h=0;h<Config::HEIGHT_COUNT;h++)assert(current.pixels[h][channel]==current.pixels[Config::LED_COUNT-1-h][channel]);
   }
   if(sameFrame(previous,current)){hold+=16;if(hold>maxHold)maxHold=hold;}else{hold=0;changes++;}
   previous=current;
  }
  assert(changes>100&&maxHold<500);
 }
 // Static sleeping/solid colors stay uniform and unchanged after their
 // power fade. Rendering a fixed instant never alternates fractional codes.
 for(int mode:{Sleep,Solid,SlowColor,Rainbow,Aurora,Ocean,Forest,Sunset,ColorWaves}){
  settings.light.mode=mode;settings.light.r=71;settings.light.g=137;settings.light.b=193;
  Renderer renderer;renderer.begin(settings,0);renderer.frame(settings,timer,2000,previous.pixels);
  for(int repeat=0;repeat<100;repeat++){renderer.frame(settings,timer,2000,current.pixels);assert(sameFrame(current,previous));}
  if(mode==Sleep||mode==Solid){
   assert(!hasSpatialVariation(previous));
   for(uint32_t now=2016;now<10000;now+=16){renderer.frame(settings,timer,now,current.pixels);assert(sameFrame(current,previous));}
  }
 }
 // The first and last hue sectors join without a full-strip color jump.
 settings.light.mode=SlowColor;settings.light.speed=100;
 CapturedFrame left=capture(settings,7992),right=capture(settings,8008);
 for(int channel=0;channel<3;channel++)assert(std::abs(int(channelTotal(left,channel))-int(channelTotal(right,channel)))<=2);
 // Before multiplying color, full gain is capped to 76 rather than 76.5.
 // A color approaching a primary therefore retains its final fractional step.
 settings.light.brightness=100;
 CapturedFrame nearPeak=capture(settings,1327);
 assert(channelTotal(nearPeak,0)==76u*Config::LED_COUNT);
 assert(channelTotal(nearPeak,1)<76u*Config::LED_COUNT-2);
 for(auto& row:nearPeak.pixels)for(auto value:row)assert(value<=76);
 std::cout<<"PASS: color aggregate step <=0.1 PWM/16ms at brightness5; fixed spatial rounding without temporal flicker\n";
}
static void testRoundingFadeAndSpeedPhase(){
 Settings settings;settings.power=true;settings.light.brightness=5;settings.light.r=71;settings.light.g=137;settings.light.b=193;
 Timer timer;
 for(int oldMode:{Sleep,Solid})for(int newMode:{SlowColor,Rainbow,Aurora,Ocean,Forest,Sunset,ColorWaves}){
  settings.light.mode=oldMode;Renderer renderer;renderer.begin(settings,0);CapturedFrame before,after;
  renderer.frame(settings,timer,1400,before.pixels);settings.light.mode=newMode;
  renderer.retarget(settings,1400);renderer.frame(settings,timer,1400,after.pixels);assert(sameFrame(before,after));
  // A second mode switch during the transition must retain the already
  // blended thresholds, including a fractional color at low brightness.
  renderer.frame(settings,timer,1700,before.pixels);settings.light.mode=oldMode;
  renderer.retarget(settings,1700);renderer.frame(settings,timer,1700,after.pixels);assert(sameFrame(before,after));
  renderer.frame(settings,timer,1700+Config::FADE_MS,after.pixels);assert(!hasSpatialVariation(after));
 }
 settings.light.brightness=100;settings.light.r=settings.light.g=settings.light.b=255;
 for(int mode=0;mode<ModeCount;mode++){
  if(cyclePeriod(mode,1)<=0)continue;
  settings.light.mode=mode;settings.light.speed=1;
  uint32_t oldPeriod=cycleMillis(cyclePeriod(mode,1)),newPeriod=cycleMillis(cyclePeriod(mode,100));
  // Change rate in cycle 3, at phase 0.37. Twinkle must retain both its
  // current cluster identity and the phase of that cluster's pulse.
  uint32_t changeAt=oldPeriod*3+oldPeriod*37/100;
  Renderer renderer;renderer.begin(settings,0);CapturedFrame before,after;
  renderer.frame(settings,timer,changeAt,before.pixels);settings.light.speed=100;
  renderer.retarget(settings,changeAt);renderer.frame(settings,timer,changeAt,after.pixels);assert(sameFrame(before,after));
  renderer.frame(settings,timer,changeAt+Config::FADE_MS,after.pixels);
  CapturedFrame expected=capture(settings,newPeriod*3+newPeriod*37/100+Config::FADE_MS);
  for(int i=0;i<Config::LED_COUNT;i++)for(int channel=0;channel<3;channel++)assert(std::abs(int(after.pixels[i][channel])-int(expected.pixels[i][channel]))<=1);
 }
 // After changing the fastest hue cycle to the slowest, the settled color
 // remains near the visible color instead of jumping to a fresh red phase.
 settings.light.mode=SlowColor;settings.light.speed=100;
 Renderer slow;slow.begin(settings,0);CapturedFrame before,after;slow.frame(settings,timer,7777,before.pixels);
 settings.light.speed=1;slow.retarget(settings,7777);slow.frame(settings,timer,7777+Config::FADE_MS,after.pixels);
 for(int i=0;i<Config::LED_COUNT;i++)for(int channel=0;channel<3;channel++)assert(std::abs(int(after.pixels[i][channel])-int(before.pixels[i][channel]))<=4);
 // Candle does not use speed; adjusting the slider must not reset its time.
 settings.light.mode=Candle;settings.light.speed=1;Renderer candle;candle.begin(settings,0);
 candle.frame(settings,timer,7777,before.pixels);settings.light.speed=100;candle.retarget(settings,7777);
 candle.frame(settings,timer,7777+Config::FADE_MS,after.pixels);assert(sameFrame(after,capture(settings,7777+Config::FADE_MS)));
}
static void testTwinkleClusters(){
 Settings settings;settings.power=true;settings.light.mode=Twinkle;settings.light.r=settings.light.g=settings.light.b=255;
 for(int speed:{1,50,100})for(int brightness:{5,50,100}){
  settings.light.speed=speed;settings.light.brightness=brightness;
  uint32_t period=cycleMillis(cyclePeriod(Twinkle,speed));assert(period>=1600);
  bool used[Config::HEIGHT_COUNT]={};int last=-1;
  for(unsigned step=0;step<10;step++){
   CapturedFrame peak=capture(settings,step*period+period/2);
   int brightest=-1,maximum=0,active=0,first=-1,final=-1;
   for(int h=0;h<Config::HEIGHT_COUNT;h++){
    int value=peak.pixels[h][0];
    if(value>maximum){maximum=value;brightest=h;}
    if(value){active++;if(first<0)first=h;final=h;}
    for(int channel=0;channel<3;channel++)assert(peak.pixels[h][channel]==peak.pixels[Config::LED_COUNT-1-h][channel]);
   }
   assert(maximum>=5&&active>=1&&active<=3&&final-first<=2);
   assert(brightest>=0&&!used[brightest]);used[brightest]=true;
   if(last>=0)assert(std::abs(brightest-last)>=3);
   last=brightest;
   // The previous cluster disappears fully before the new one grows;
   // dark background makes its small bright patch visually distinct.
   CapturedFrame boundary=capture(settings,(step+1)*period);assert(frameTotal(boundary)==0);
   for(uint32_t at=period/4;at<period;at+=113){
    CapturedFrame frame=capture(settings,step*period+at);int lit=0;
    for(int h=0;h<Config::HEIGHT_COUNT;h++)if(frame.pixels[h][0])lit++;
    assert(lit<=3);
   }
  }
  for(bool visited:used)assert(visited);
 }
}
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
 testNewEffectCommands();testNewEffectRendering();
 testColorQuantization();testRoundingFadeAndSpeedPhase();testTwinkleClusters();
 Renderer render;Settings s;s.power=true;s.light.mode=Solid;s.light.r=s.light.g=s.light.b=255;s.light.brightness=100;Timer noTimer;uint8_t frame[Config::LED_COUNT][3];render.begin(s,0);render.frame(s,noTimer,800,frame);assert(frame[0][0]==76);
 s.power=false;render.retarget(s,800);render.frame(s,noTimer,1600,frame);for(auto& row:frame)for(auto x:row)assert(x==0);
 s.power=true;s.light.mode=Rise;render.begin(s,0);for(uint32_t n=800;n<20000;n+=33){render.frame(s,noTimer,n,frame);for(int i=0;i<10;i++)for(int ch=0;ch<3;ch++)assert(frame[i][ch]==frame[19-i][ch]);}
 s.light.mode=Sleep;s.light.brightness=5;render.begin(s,0);render.frame(s,noTimer,800,frame);assert(frame[0][0]>frame[0][1]&&frame[0][1]>0&&frame[0][2]>0);
 // Cancelling a timer in its fade must start from the displayed brightness.
 s.light.mode=Solid;s.light.brightness=100;s.light.r=s.light.g=s.light.b=255;render.begin(s,0);t.begin(0,1);render.frame(s,t,30000,frame);int faded=frame[0][0];t.cancel();render.retarget(s,30000);render.frame(s,t,30000,frame);assert(frame[0][0]==faded);render.frame(s,t,30800,frame);assert(frame[0][0]==76);
 s.light.mode=Candle;render.begin(s,0);for(uint32_t n=800;n<60000;n+=33){render.frame(s,noTimer,n,frame);for(auto& row:frame)for(auto x:row)assert(x<=76);}
 std::cout<<"PASS: controller/favorites/tint, 14 effects/speed/determinism/continuity/pairing/cap/fades, sleep memory, timer rollover, touch/holds/multi-touch and WiFi reset chord\n";
}
