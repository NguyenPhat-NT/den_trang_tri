#pragma once
#include "Config.h"
#include <math.h>
#include <stdint.h>
namespace Deco {
// IDs 0..5 are persisted by previous firmware and must remain unchanged.
enum Mode : uint8_t { Sleep=0, Solid=1, SlowColor=2, Candle=3, Rainbow=4, Rise=5,
 Breath=6, Twinkle=7, Aurora=8, Ocean=9, Forest=10, Sunset=11, Comet=12, ColorWaves=13, ModeCount=14 };
inline bool supportsTint(uint8_t mode){return mode==Solid||mode==Breath||mode==Twinkle||mode==Comet;}
struct Light {
 uint8_t mode=Sleep, r=255,g=96,b=24,brightness=5,speed=50;
 bool operator==(const Light& b) const {return mode==b.mode&&r==b.r&&g==b.g&&this->b==b.b&&brightness==b.brightness&&speed==b.speed;}
};
struct Favorite {bool valid=false; Light light; bool operator==(const Favorite& b)const{return valid==b.valid&&light==b.light;}};
struct Settings {
 bool power=false; Light light; uint8_t sleepBrightness=5,cap=30; Favorite favorites[3];
 bool operator==(const Settings& b)const {
  if(power!=b.power||!(light==b.light)||sleepBrightness!=b.sleepBrightness||cap!=b.cap)return false;
  for(int i=0;i<3;i++)if(!(favorites[i]==b.favorites[i]))return false;
  return true;
 }
};
inline bool validLight(const Light& l){return l.mode<ModeCount&&l.brightness>=1&&l.brightness<=100&&l.speed>=1&&l.speed<=100;}
inline bool validSettings(const Settings& s){
 if(!validLight(s.light)||s.sleepBrightness<1||s.sleepBrightness>100||s.cap<Config::MIN_CAP_PERCENT||s.cap>Config::MAX_CAP_PERCENT)return false;
 for(auto& f:s.favorites)if(!validLight(f.light))return false;
 return true;
}
inline uint8_t clampBrightness(int n){return n<1?1:n>100?100:n;}
inline float clamp01(float n){return n<0?0:n>1?1:n;}
inline float scale(float p,uint8_t cap){float u=clamp01((p-1.0f)/99.0f);return Config::MIN_OUTPUT+(cap/100.0f-Config::MIN_OUTPUT)*powf(u,Config::GAMMA);}
struct Timer {
 bool active=false; uint32_t start=0,duration=0;
 void begin(uint32_t now,uint16_t minutes){active=true;start=now;duration=uint32_t(minutes)*60000u;}
 void cancel(){active=false;}
 uint32_t remaining(uint32_t now)const {if(!active)return 0;uint32_t dt=now-start;return dt>=duration?0:duration-dt;}
 float envelope(uint32_t now)const {if(!active)return 1;uint32_t tail=duration<300000u?duration:300000u;uint32_t left=remaining(now);return left>=tail?1:float(left)/tail;}
};
enum class Action {Power,Brightness,Mode,Color,Speed,SleepNow,Cap,FavoriteSave,FavoriteApply,TimerStart,TimerCancel,Tint};
struct Command {Action action; int value;uint8_t r,g,b; Command(Action a,int v=0,uint8_t red=0,uint8_t green=0,uint8_t blue=0):action(a),value(v),r(red),g(green),b(blue){} };
struct Result {bool accepted,changed,feedback;Result(bool a=true,bool c=false,bool f=false):accepted(a),changed(c),feedback(f){} };
class Controller {
 public:
 Settings settings; Timer timer; uint32_t revision=0;
 Result apply(const Command& c,uint32_t now){
  Settings before=settings; bool oldTimer=timer.active;uint32_t oldStart=timer.start,oldDuration=timer.duration;
  bool cancelOnChange=true, feedback=false;
  switch(c.action){
   case Action::Power: if(c.value<0||c.value>1)return {false,false,false}; settings.power=c.value; if(!settings.power)timer.cancel();break;
   case Action::Brightness: if(c.value<1||c.value>100)return {false,false,false};settings.light.brightness=c.value;if(settings.light.mode==Sleep)settings.sleepBrightness=c.value;break;
   case Action::Mode: if(c.value<0||c.value>=ModeCount)return {false,false,false};if(settings.light.mode!=c.value){settings.light.mode=c.value;if(c.value==Sleep)settings.light.brightness=settings.sleepBrightness;}break;
   case Action::Color: settings.light.mode=Solid;settings.light.r=c.r;settings.light.g=c.g;settings.light.b=c.b;break;
   case Action::Tint: if(!supportsTint(settings.light.mode))return {false,false,false};settings.light.r=c.r;settings.light.g=c.g;settings.light.b=c.b;break;
   case Action::Speed: if(c.value<1||c.value>100)return {false,false,false};settings.light.speed=c.value;break;
   case Action::SleepNow: settings.power=true;settings.light.mode=Sleep;settings.light.brightness=settings.sleepBrightness;break;
   case Action::Cap: if(c.value<Config::MIN_CAP_PERCENT||c.value>Config::MAX_CAP_PERCENT)return {false,false,false};settings.cap=c.value;break;
   case Action::FavoriteSave: if(c.value<0||c.value>2)return {false,false,false};settings.favorites[c.value].valid=true;settings.favorites[c.value].light=settings.light;cancelOnChange=false;break;
   case Action::FavoriteApply: if(c.value<0||c.value>2||!settings.favorites[c.value].valid)return {false,false,false};settings.light=settings.favorites[c.value].light;if(settings.light.mode==Sleep)settings.sleepBrightness=settings.light.brightness;break;
   case Action::TimerStart: if(!settings.power||c.value<1||c.value>180)return {false,false,false};timer.begin(now,c.value);cancelOnChange=false;feedback=true;break;
   case Action::TimerCancel: feedback=timer.active;timer.cancel();cancelOnChange=false;break;
  }
  bool changed=!(before==settings);
  if(changed&&cancelOnChange)timer.cancel();
  if(changed||timer.active!=oldTimer||timer.start!=oldStart||timer.duration!=oldDuration)revision++;
  return {true,changed,feedback};
 }
 Result tick(uint32_t now){if(timer.active&&timer.remaining(now)==0)return apply({Action::Power,0},now);return {};}
};
enum class TouchEvent {PowerShort,SleepHold,MinusShort,MinusStep,PlusShort,PlusStep,ModeShort,TimerHold,WiFiResetHold};
class Touch {
 struct Button {bool raw=false,down=false,held=false;uint32_t rawAt=0,pressedAt=0,repeatedAt=0;};
 Button buttons[4];uint32_t boot=0,resetAt=0;bool ready=false,blocked=false,resetPending=false,resetStarted=false,resetFired=false;
 public:
 void begin(uint32_t now){boot=now;ready=false;blocked=resetPending=resetStarted=resetFired=false;for(auto& b:buttons)b=Button{};}
 template<class Emit> void scan(const bool raw[4],uint32_t now,Emit emit){
  uint8_t rawMask=0,downMask=0;
  for(int i=0;i<4;i++){
   auto& b=buttons[i];if(raw[i])rawMask|=uint8_t(1u<<i);
   if(b.raw!=raw[i]){b.raw=raw[i];b.rawAt=now;}
   if(b.raw!=b.down&&uint32_t(now-b.rawAt)>=Config::DEBOUNCE_MS){
    b.down=b.raw;
    if(b.down){b.pressedAt=now;b.repeatedAt=now;b.held=false;}
   }
   if(b.down)downMask|=uint8_t(1u<<i);
  }
  if(!ready){if(uint32_t(now-boot)>=Config::TOUCH_STARTUP_MS&&rawMask==0&&downMask==0){ready=true;for(auto& b:buttons)b.held=true;}return;}
  if(blocked){
   // Newly debounced presses must also be consumed while waiting for a full release.
   for(auto& b:buttons)b.held=true;
   if(rawMask==0&&downMask==0)blocked=false;
   return;
  }
  constexpr uint8_t resetMask=(1u<<1)|(1u<<2);
  if(resetPending||rawMask==resetMask){
   // Start timing only when both + and - have passed debounce. Any other
   // input or a physical release cancels the entire gesture, even before
   // release debounce finishes, so no single-button action can escape it.
   for(auto& b:buttons)b.held=true;
   if(rawMask!=resetMask||(downMask&uint8_t(~resetMask))!=0){
    blocked=true;resetPending=resetStarted=resetFired=false;
   }else{
    resetPending=true;
    if(downMask==resetMask){
     if(!resetStarted){resetStarted=true;resetAt=now;}
     else if(!resetFired&&uint32_t(now-resetAt)>=Config::WIFI_RESET_HOLD_MS){resetFired=true;emit(TouchEvent::WiFiResetHold);}
    }
   }
   return;
  }
  if((rawMask&&(rawMask&(rawMask-1)))||(downMask&&(downMask&(downMask-1)))){
   blocked=true;for(auto& b:buttons)b.held=true;return;
  }
  for(int i=0;i<4;i++){
   auto& b=buttons[i];
   if(!b.down){
    if(!b.held&&uint32_t(now-b.rawAt)>=Config::DEBOUNCE_MS){b.held=true;emit(i==0?TouchEvent::PowerShort:i==1?TouchEvent::MinusShort:i==2?TouchEvent::PlusShort:TouchEvent::ModeShort);}
    continue;
   }
   // Do not trigger a hold after a physical release awaiting debounce.
   if(!b.raw)continue;
   uint32_t dt=now-b.pressedAt;
   if(i==1||i==2){
    if(dt>=Config::REPEAT_START_MS&&(!b.held||uint32_t(now-b.repeatedAt)>=Config::REPEAT_MS)){
     b.held=true;b.repeatedAt=now;emit(i==1?TouchEvent::MinusStep:TouchEvent::PlusStep);
    }
   }else if(dt>=Config::LONG_HOLD_MS&&!b.held){b.held=true;emit(i==0?TouchEvent::SleepHold:TouchEvent::TimerHold);}
  }
 }
};
struct Color {float r,g,b;Color(float red=0,float green=0,float blue=0):r(red),g(green),b(blue){} };
inline Color hsv(float h){h=h-floorf(h);float x=h*6;int i=int(x);float f=x-i;switch(i){case 0:return {1,f,0};case 1:return {1-f,1,0};case 2:return {0,1,f};case 3:return {0,1-f,1};case 4:return {f,0,1};default:return {1,0,1-f};}}
inline Color colorMix(const Color& a,const Color& b,float amount){return {a.r+(b.r-a.r)*amount,a.g+(b.g-a.g)*amount,a.b+(b.b-a.b)*amount};}
inline Color colorGain(const Color& color,float amount){return {color.r*amount,color.g*amount,color.b*amount};}
inline float wave(float cycles){return .5f+.5f*sinf(cycles*6.28318531f);}
inline uint32_t cycleMillis(float seconds){return uint32_t(seconds*1000.0f+.5f);}
inline float cyclePhase(uint32_t elapsed,float seconds){uint32_t period=cycleMillis(seconds);return float(elapsed%period)/period;}
inline float cyclePeriod(uint8_t mode,uint8_t rate){
 float speed=(rate-1)/99.0f;
 switch(mode){
  case SlowColor:return 120.0f-112.0f*speed;
  case Rainbow:return 120.0f-108.0f*speed;
  case Rise:return 8.0f-7.0f*speed;
  case Breath:return 16.0f-12.0f*speed;
  case Twinkle:return 4.8f-3.2f*speed;
  case Aurora:return 45.0f-35.0f*speed;
  case Ocean:return 30.0f-23.0f*speed;
  case Forest:return 50.0f-38.0f*speed;
  case Sunset:return 60.0f-46.0f*speed;
  case Comet:return 24.0f-18.0f*speed;
  case ColorWaves:return 32.0f-24.0f*speed;
  default:return 0;
 }
}
inline bool spatialRounding(uint8_t mode){return mode==SlowColor||mode==Rainbow||mode==Aurora||mode==Ocean||mode==Forest||mode==Sunset||mode==ColorWaves;}
inline Color roundingThreshold(uint8_t mode,uint16_t height){
 if(!spatialRounding(mode))return {.5f,.5f,.5f};
 // One fixed threshold per paired height, with a separate ordering for
 // each channel. This spreads fractional levels spatially, never in time.
 return {(float((height*3u)%Config::HEIGHT_COUNT)+.5f)/Config::HEIGHT_COUNT,
         (float((height*3u+2u)%Config::HEIGHT_COUNT)+.5f)/Config::HEIGHT_COUNT,
         (float((height*3u+4u)%Config::HEIGHT_COUNT)+.5f)/Config::HEIGHT_COUNT};
}
class Renderer {
 Color from[Config::LED_COUNT],current[Config::LED_COUNT];Light target;
 Color roundFrom[Config::HEIGHT_COUNT],roundCurrent[Config::HEIGHT_COUNT];
 float pFrom=5,pCurrent=5,pTarget=5,gateFrom=0,gateCurrent=0,gateTarget=0,lastTimerEnvelope=1;
 float phaseOffset=0;uint8_t twinkleStepOffset=0;
 uint32_t transitionAt=0,phaseAt=0,feedbackAt=0;bool feedbackActive=false;
 float phase(const Light& l,uint32_t now)const{
  float unit=phaseOffset+cyclePhase(uint32_t(now-phaseAt),cyclePeriod(l.mode,l.speed));
  return unit-floorf(unit);
 }
 uint8_t twinkleStep(const Light& l,uint32_t now)const{
  uint32_t elapsed=now-phaseAt,period=cycleMillis(cyclePeriod(l.mode,l.speed));
  uint32_t carry=phaseOffset+float(elapsed%period)/period>=1.0f?1u:0u;
  return uint8_t((twinkleStepOffset+elapsed/period+carry)%10u);
 }
 Color effect(const Light& l,uint16_t i,uint32_t now)const{
  uint32_t elapsed=now-phaseAt;float t=float(elapsed)/1000.0f;
  uint16_t height=i<Config::HEIGHT_COUNT?i:Config::LED_COUNT-1-i;
  float z=Config::HEIGHT_COUNT>1?float(height)/(Config::HEIGHT_COUNT-1):0;
  Color tint(l.r/255.0f,l.g/255.0f,l.b/255.0f);
  float cycle=cyclePeriod(l.mode,l.speed)>0?phase(l,now):0;
  switch(l.mode){
   case Sleep:return {Config::SLEEP_R/255.0f,Config::SLEEP_G/255.0f,Config::SLEEP_B/255.0f};
   case Solid:return {l.r/255.0f,l.g/255.0f,l.b/255.0f};
   case SlowColor:return hsv(cycle); // linear hue, 120..8 seconds per cycle
   case Candle:{float f=.9f+.045f*sinf(t*4.3f)+.035f*sinf(t*7.7f+float(i)*.23f)+.02f*sinf(t*13.1f);return {f,.37647f*f,.09412f*f};}
   case Rainbow:return hsv(float(i)/Config::LED_COUNT+cycle);
   case Rise:{float pos=cycle*(Config::HEIGHT_COUNT+3)-2.0f;float f=clamp01(1-fabsf(height-pos)/2.0f);return {f,.37647f*f,.09412f*f};}
   case Breath:{
    float pulse=.5f-.5f*cosf(cycle*6.28318531f);
    return colorGain(tint,.16f+.84f*pulse);
   }
   case Twinkle:{
    // A single soft cluster moves to a separated height only while dark.
    // Fixed ordering is deterministic and keeps most of the lamp unlit.
    static const uint8_t centers[10]={0,6,2,8,4,9,3,7,1,5};
    float center=centers[twinkleStep(l,now)]*(Config::HEIGHT_COUNT-1)/9.0f;
    float distance=fabsf(height-center),spatial=distance<1.5f?.5f+.5f*cosf(distance*3.14159265f/1.5f):0;
    float bloom=sinf(cycle*3.14159265f);bloom*=bloom;
    return colorGain(tint,.005f+.995f*spatial*bloom);
   }
   case Aurora:{
    Color ribbons=colorMix({.02f,.85f,.38f},{.38f,.04f,.88f},wave(z*.85f-cycle));
    ribbons=colorMix(ribbons,{.08f,.55f,.85f},.40f*wave(z*1.6f+cycle*2.0f+.2f));
    return colorGain(ribbons,.30f+.70f*wave(z*.65f-cycle+.15f));
   }
   case Ocean:{
    float swell=wave(z*1.25f-cycle),foam=wave(z*2.0f-cycle*2.0f+.18f);
    Color sea=colorMix({.01f,.04f,.30f},{.02f,.62f,.80f},swell);
    sea=colorMix(sea,{.30f,.72f,.92f},.28f*foam*foam);
    return colorGain(sea,.55f+.45f*swell);
   }
   case Forest:{
    Color leaves=colorMix({.02f,.30f,.06f},{.30f,.65f,.04f},wave(z-cycle));
    float sun=wave(z*1.75f-cycle*2.0f+.2f);sun*=sun;
    leaves=colorMix(leaves,{.65f,.42f,.06f},.30f*sun);
    return colorGain(leaves,.55f+.45f*wave(z-cycle+.3f));
   }
   case Sunset:{
    Color sky=colorMix({1.0f,.38f,.04f},{.90f,.08f,.24f},clamp01(z*.70f+.30f*wave(cycle)));
    sky=colorMix(sky,{.15f,.025f,.40f},z*z*(.20f+.40f*wave(cycle+.2f)));
    return colorGain(sky,.70f+.30f*wave(z-cycle));
   }
   case Comet:{
    // The head travels beyond both ends before looping; its soft tail is
    // nearly dark there, keeping the restart gentle on a short LED strip.
    float head=cycle*(Config::HEIGHT_COUNT+12.0f)-6.0f,d=head-height;
    float glow=expf(-d*d/1.6f),tail=d>=0?expf(-d/2.0f):expf(d*2.5f);
    return colorGain(tint,.02f+.98f*(.85f*glow+.15f*tail));
   }
   case ColorWaves:{
    Color colors=colorMix(hsv(z-cycle),{.10f,.12f,.18f},.15f);
    return colorGain(colors,.40f+.60f*wave(z*1.5f-cycle));
   }
   default:return {};
  }
 }
 public:
 void begin(const Settings& s,uint32_t now){
  target=s.light;pFrom=pCurrent=pTarget=s.light.brightness;gateFrom=gateCurrent=0;gateTarget=s.power?1:0;transitionAt=phaseAt=now;phaseOffset=0;twinkleStepOffset=0;
  for(uint16_t i=0;i<Config::LED_COUNT;i++)from[i]=current[i]=effect(target,i,now);
  for(uint16_t h=0;h<Config::HEIGHT_COUNT;h++)roundFrom[h]=roundCurrent[h]=roundingThreshold(target.mode,h);
 }
 void retarget(const Settings& s,uint32_t now){
  for(uint16_t i=0;i<Config::LED_COUNT;i++)from[i]=current[i];
  for(uint16_t h=0;h<Config::HEIGHT_COUNT;h++)roundFrom[h]=roundCurrent[h];
  pFrom=pCurrent;pTarget=s.light.brightness;gateFrom=gateCurrent*lastTimerEnvelope;gateTarget=s.power?1:0;
  if(target.mode!=s.light.mode){phaseAt=now;phaseOffset=0;twinkleStepOffset=0;}
  else if(target.speed!=s.light.speed&&cyclePeriod(target.mode,target.speed)>0){
   uint8_t step=target.mode==Twinkle?twinkleStep(target,now):0;
   phaseOffset=phase(target,now);twinkleStepOffset=step;phaseAt=now;
  }
  target=s.light;transitionAt=now;
 }
 void feedback(uint32_t now){feedbackAt=now;feedbackActive=true;}
 void frame(const Settings& s,const Timer& timer,uint32_t now,uint8_t out[Config::LED_COUNT][3]){
  float k=clamp01(float(uint32_t(now-transitionAt))/Config::FADE_MS);
  pCurrent=pFrom+(pTarget-pFrom)*k;gateCurrent=gateFrom+(gateTarget-gateFrom)*k;
  float pulse=1;if(feedbackActive){float f=float(uint32_t(now-feedbackAt))/400.0f;if(f>=1)feedbackActive=false;else pulse=1-.15f*sinf(f*3.14159265f);}
  lastTimerEnvelope=timer.envelope(now);
  int maxCode=int(floorf(s.cap/100.0f*255.0f));
  float peakCodes=fminf(scale(pCurrent,s.cap)*255,maxCode);
  float gainCodes=peakCodes*gateCurrent*lastTimerEnvelope*pulse;
  for(uint16_t h=0;h<Config::HEIGHT_COUNT;h++)roundCurrent[h]=colorMix(roundFrom[h],roundingThreshold(target.mode,h),k);
  for(uint16_t i=0;i<Config::LED_COUNT;i++){
   Color e=effect(target,i,now);current[i]={from[i].r+(e.r-from[i].r)*k,from[i].g+(e.g-from[i].g)*k,from[i].b+(e.b-from[i].b)*k};
   float c[3]={current[i].r,current[i].g,current[i].b};
   uint16_t height=i<Config::HEIGHT_COUNT?i:Config::LED_COUNT-1-i;
   float threshold[3]={roundCurrent[height].r,roundCurrent[height].g,roundCurrent[height].b};
   for(int j=0;j<3;j++){int code=int(floorf(clamp01(c[j])*gainCodes+1.0f-threshold[j]));out[i][j]=code>maxCode?maxCode:code<0?0:code;}
  }
 }
};
}
