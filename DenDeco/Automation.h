#pragma once
#include "Core.h"
#include "Schedule.h"
#include "Solar.h"
namespace Deco {
enum class AutomationKind {None, WeeklyOn, WeeklyOff, Dawn, Dusk};
// Collect events before touching the lamp: an OFF collision cannot briefly
// show an ON preset. Later weekly rows win among ON events.
struct AutomationChoice {
 AutomationKind kind=AutomationKind::None;int8_t favorite=-1;
 SolarEntry solar;int64_t eventUtc=0;
 void weekly(uint8_t,const ScheduleEntry& entry){
  if(!entry.power){kind=AutomationKind::WeeklyOff;favorite=-1;}
  else if(kind!=AutomationKind::WeeklyOff){kind=AutomationKind::WeeklyOn;favorite=entry.favorite;}
 }
 void sun(bool dawn,const SolarEntry& entry,int64_t utc){
  if(kind==AutomationKind::WeeklyOn||kind==AutomationKind::WeeklyOff)return;
  if(dawn&&kind==AutomationKind::Dusk)return;
  kind=dawn?AutomationKind::Dawn:AutomationKind::Dusk;solar=entry;favorite=entry.favorite;eventUtc=utc;
 }
};
inline bool automationLight(const Settings& settings,const AutomationChoice& choice,Light& light){
 if(choice.kind==AutomationKind::None||choice.kind==AutomationKind::WeeklyOff)return false;
 if(choice.favorite>=0){
  if(choice.favorite>2||!settings.favorites[choice.favorite].valid||!validLight(settings.favorites[choice.favorite].light))return false;
  light=settings.favorites[choice.favorite].light;
 }else{
  if(choice.kind==AutomationKind::WeeklyOn)return false;
  light.mode=Solid;light.r=255;light.g=160;light.b=72;light.brightness=choice.solar.brightness;light.speed=50;
 }
 return validLight(light);
}
struct BrightnessRamp {
 bool active=false;float from=0,target=0;uint32_t start=0,duration=0;
 void begin(float initial,float goal,uint32_t now,uint32_t length){active=true;from=fmaxf(0,fminf(initial,100));target=fmaxf(0,fminf(goal,100));start=now;duration=length;}
 float value(uint32_t now)const{float progress=duration?clamp01(float(uint32_t(now-start))/duration):1;return from+(target-from)*progress;}
 uint32_t remaining(uint32_t now)const{uint32_t elapsed=now-start;return !active||elapsed>=duration?0:duration-elapsed;}
 void cancel(){active=false;}
};
// Ordinary second boundaries and hourly SNTP refreshes do not disarm a due
// minute. A real wall-clock step relative to millis() does.
class ClockContinuity {
 bool observed=false;int64_t lastUtc=0;uint32_t lastMillis=0;
 public:
 bool observe(int64_t utc,bool ready,uint32_t now){
  ScheduleWallTime wall;if(!ready||!localScheduleTime(utc,wall)){observed=false;return false;}
  bool jumped=false;
  if(observed){int64_t error=(utc-lastUtc)*1000-int64_t(uint32_t(now-lastMillis));jumped=error>2000||error<-2000;}
  observed=true;lastUtc=utc;lastMillis=now;return jumped;
 }
};
}
