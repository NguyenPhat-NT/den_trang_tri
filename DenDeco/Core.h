#pragma once
#include "Config.h"
#include <math.h>
#include <stdint.h>
namespace Deco {
enum Mode : uint8_t { Sleep=0, Solid, SlowColor, Candle, Rainbow, Rise, ModeCount };
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
enum class Action {Power,Brightness,Mode,Color,Speed,SleepNow,Cap,FavoriteSave,FavoriteApply,TimerStart,TimerCancel};
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
class Renderer {
 Color from[Config::LED_COUNT],current[Config::LED_COUNT];Light target;
 float pFrom=5,pCurrent=5,pTarget=5,gateFrom=0,gateCurrent=0,gateTarget=0,lastTimerEnvelope=1;
 uint32_t transitionAt=0,phaseAt=0,feedbackAt=0;bool feedbackActive=false;
 Color effect(const Light& l,uint16_t i,uint32_t now)const{
  float t=float(uint32_t(now-phaseAt))/1000.0f;float speed=(l.speed-1)/99.0f;
  switch(l.mode){
   case Sleep:return {Config::SLEEP_R/255.0f,Config::SLEEP_G/255.0f,Config::SLEEP_B/255.0f};
   case Solid:return {l.r/255.0f,l.g/255.0f,l.b/255.0f};
   case SlowColor:return hsv(t/(120.0f-119.0f*speed)); // speed 50: about 61 seconds
   case Candle:{float f=.9f+.045f*sinf(t*4.3f)+.035f*sinf(t*7.7f+float(i)*.23f)+.02f*sinf(t*13.1f);return {f,.37647f*f,.09412f*f};}
   case Rainbow:return hsv(float(i)/Config::LED_COUNT+t/(120.0f-115.0f*speed));
   case Rise:{uint16_t h=i<Config::HEIGHT_COUNT?i:Config::LED_COUNT-1-i;float pos=fmodf(t/(8.0f-7.0f*speed),1.0f)*(Config::HEIGHT_COUNT+3)-2.0f;float f=clamp01(1-fabsf(h-pos)/2.0f);return {f,.37647f*f,.09412f*f};}
   default:return {};
  }
 }
 public:
 void begin(const Settings& s,uint32_t now){target=s.light;pFrom=pCurrent=pTarget=s.light.brightness;gateFrom=gateCurrent=0;gateTarget=s.power?1:0;transitionAt=phaseAt=now;for(uint16_t i=0;i<Config::LED_COUNT;i++)from[i]=current[i]=effect(target,i,now);}
 void retarget(const Settings& s,uint32_t now){for(uint16_t i=0;i<Config::LED_COUNT;i++)from[i]=current[i];pFrom=pCurrent;pTarget=s.light.brightness;gateFrom=gateCurrent*lastTimerEnvelope;gateTarget=s.power?1:0;if(target.mode!=s.light.mode)phaseAt=now;target=s.light;transitionAt=now;}
 void feedback(uint32_t now){feedbackAt=now;feedbackActive=true;}
 void frame(const Settings& s,const Timer& timer,uint32_t now,uint8_t out[Config::LED_COUNT][3]){
  float k=clamp01(float(uint32_t(now-transitionAt))/Config::FADE_MS);
  pCurrent=pFrom+(pTarget-pFrom)*k;gateCurrent=gateFrom+(gateTarget-gateFrom)*k;
  float pulse=1;if(feedbackActive){float f=float(uint32_t(now-feedbackAt))/400.0f;if(f>=1)feedbackActive=false;else pulse=1-.15f*sinf(f*3.14159265f);}
  lastTimerEnvelope=timer.envelope(now);
  float gain=scale(pCurrent,s.cap)*gateCurrent*lastTimerEnvelope*pulse;
  int maxCode=int(floorf(s.cap/100.0f*255.0f));
  for(uint16_t i=0;i<Config::LED_COUNT;i++){
   Color e=effect(target,i,now);current[i]={from[i].r+(e.r-from[i].r)*k,from[i].g+(e.g-from[i].g)*k,from[i].b+(e.b-from[i].b)*k};
   float c[3]={current[i].r,current[i].g,current[i].b};
   for(int j=0;j<3;j++){int code=int(lroundf(clamp01(c[j])*gain*255));out[i][j]=code>maxCode?maxCode:code<0?0:code;}
  }
 }
};
}
