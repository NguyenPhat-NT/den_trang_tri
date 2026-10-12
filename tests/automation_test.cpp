#include "../DenDeco/Automation.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace Deco;
static void testCollisionsAndTargets(){
 Settings settings;Light target;SolarEntry sun;sun.enabled=true;
 AutomationChoice choice;choice.sun(true,sun,100);choice.sun(false,sun,200);choice.sun(true,sun,100);
 assert(choice.kind==AutomationKind::Dusk&&choice.eventUtc==200);
 assert(automationLight(settings,choice,target)&&target.mode==Solid&&target.r==255&&target.g==160&&target.b==72&&target.brightness==35);
 ScheduleEntry on;on.enabled=true;on.days=127;on.power=true;on.favorite=1;
 choice.weekly(0,on);choice.sun(false,sun,200);assert(choice.kind==AutomationKind::WeeklyOn&&choice.favorite==1);
 assert(!automationLight(settings,choice,target)); // Do not substitute an unexpected preset.
 settings.favorites[1].valid=true;settings.favorites[1].light.mode=Ocean;settings.favorites[1].light.brightness=72;
 assert(automationLight(settings,choice,target)&&target.mode==Ocean&&target.brightness==72);
 on.favorite=2;choice.weekly(7,on);assert(choice.favorite==2);
 ScheduleEntry off;off.enabled=true;off.days=127;
 choice.weekly(1,off);choice.weekly(7,on);choice.sun(false,sun,200);
 assert(choice.kind==AutomationKind::WeeklyOff&&!automationLight(settings,choice,target));
 sun.favorite=1;AutomationChoice dawn;dawn.sun(true,sun,100);
 assert(automationLight(settings,dawn,target)&&target.brightness==72); // Live preset overrides built-in brightness.
 settings.favorites[1].light.brightness=18;assert(automationLight(settings,dawn,target)&&target.brightness==18);
 settings.favorites[1].light.mode=255;assert(!automationLight(settings,dawn,target));
}
static void testRampAndRenderer(){
 Settings settings;settings.power=true;settings.light.mode=Solid;settings.light.r=255;settings.light.g=160;settings.light.b=72;settings.light.brightness=35;
 Timer timer;Renderer renderer;BrightnessRamp ramp;
 const uint32_t start=0xffff0000u;renderer.begin(settings,start);ramp.begin(0,35,start,60000);
 uint8_t pixels[Config::LED_COUNT][3];int previous=0;
 // A 1-minute dawn must start black, rise without temporal dithering or
 // reversals, mirror both columns, and remain within the output cap.
 for(uint32_t elapsed=0;elapsed<=60000;elapsed+=16){
  uint32_t now=start+elapsed;renderer.frame(settings,timer,now,pixels,ramp.value(now));int aggregate=0;
  for(uint16_t i=0;i<Config::LED_COUNT;i++)for(int channel=0;channel<3;channel++){
   assert(pixels[i][channel]==pixels[Config::LED_COUNT-1-i][channel]);assert(pixels[i][channel]<=76);aggregate+=pixels[i][channel];
  }
  if(!elapsed)assert(aggregate==0);
  assert(aggregate>=previous);previous=aggregate;
 }
 assert(previous>0&&std::fabs(ramp.value(start+30000)-17.5f)<.001f);
 assert(ramp.remaining(start+59999)==1&&ramp.remaining(start+60000)==0);
 assert(ramp.value(start+61000)==35);
 // Manual interruption retargets from the actual rendered level rather
 // than the preset's final level, even across millis rollover.
 renderer.frame(settings,timer,start+30000,pixels,ramp.value(start+30000));
 uint8_t before[Config::LED_COUNT][3];for(uint16_t i=0;i<Config::LED_COUNT;i++)for(int c=0;c<3;c++)before[i][c]=pixels[i][c];
 settings.light.brightness=18;ramp.cancel();renderer.retarget(settings,start+30000);renderer.frame(settings,timer,start+30000,pixels);
 for(uint16_t i=0;i<Config::LED_COUNT;i++)for(int c=0;c<3;c++)assert(pixels[i][c]==before[i][c]);
 renderer.frame(settings,timer,start+31000,pixels);assert(std::fabs(renderer.brightness()-18)<.001f);
 // OFF during the dim beginning of dawn must fade down. Rounding a
 // persistable slider level up to 1 must not cause a brief bright pulse.
 Renderer early;settings.light.brightness=35;settings.power=true;early.begin(settings,0);early.frame(settings,timer,1000,pixels,.1f);
 int last=0;for(uint16_t i=0;i<Config::LED_COUNT;i++)for(int c=0;c<3;c++)last+=pixels[i][c];
 settings.light.brightness=1;settings.power=false;early.retarget(settings,1000);
 for(uint32_t elapsed=0;elapsed<=Config::FADE_MS+16;elapsed+=16){
  early.frame(settings,timer,1000+elapsed,pixels);int sum=0;
  for(uint16_t i=0;i<Config::LED_COUNT;i++)for(int c=0;c<3;c++)sum+=pixels[i][c];
  assert(sum<=last);last=sum;
 }
 assert(last==0);
 BrightnessRamp instant;instant.begin(0,10,100,0);assert(instant.value(100)==10&&instant.remaining(100)==0);
}
static void testClockContinuityAndActualScheduling(){
 ClockContinuity clock;Scheduler scheduler;ScheduleConfig config;config.enabled=true;
 auto& entry=config.entries[0];entry.enabled=true;entry.days=1;entry.minute=421;entry.power=true;entry.favorite=0;
 assert(scheduler.begin(config));const int64_t monday=1791763200LL;uint32_t now=0xffff0000u;unsigned emitted=0;
 auto emit=[&](uint8_t,const ScheduleEntry&){emitted++;};
 assert(!clock.observe(monday,false,now));scheduler.tick(monday,false,config,emit);assert(!emitted);
 // Genuine sync primes the current minute. Normal second changes and a
 // matching wall clock/millis progression do not suppress the next minute.
 assert(!clock.observe(monday,true,now));scheduler.tick(monday,true,config,emit);
 for(int second=0;second<=60;second++)for(int fraction=0;fraction<10;fraction++){
  uint32_t tick=now+second*1000+fraction*100;
  assert(!clock.observe(monday+second,true,tick));scheduler.tick(monday+second,true,config,emit);
 }
 assert(emitted==1);
 assert(clock.observe(monday+70,true,now+61000));scheduler.rebaseline(monday+70,true);scheduler.tick(monday+70,true,config,emit);assert(emitted==1);
 assert(!clock.observe(0,false,now+62000));assert(!clock.observe(monday+80,true,now+63000));
 ClockContinuity offline;assert(!offline.observe(monday,true,0));assert(!offline.observe(monday+3600,true,3600000)); // Holdover/ordinary hourly sync.
}
int main(){testCollisionsAndTargets();testRampAndRenderer();testClockContinuityAndActualScheduling();std::cout<<"PASS: atomic automation priorities/live presets, zero-to-target dawn continuity/pairing/cap/millis rollover, manual interruption, fresh-clock arming and wall-clock step detection\n";}
