#include "../DenDeco/Solar.h"
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <vector>
using namespace Deco;

static const int64_t MONDAY=20738; // Gregorian 2026-10-12 in UTC+7.
static SolarTimes timesFor(int64_t day,const SolarConfig& config){
 SolarTimes times;assert(calculateSolarTimes(day,config,times));assert(times.localDay==day);return times;
}
static int64_t due(int64_t sourceDay,const SolarConfig& config,bool dawn=true){
 SolarTimes times=timesFor(sourceDay,config);
 assert(dawn?times.sunriseValid:times.sunsetValid);
 return (dawn?times.sunriseUtc:times.sunsetUtc)+int64_t(dawn?config.dawn.offset:config.dusk.offset)*60;
}
struct Event {bool dawn;SolarEntry entry;int64_t utc;};
struct Fixture {
 SolarScheduler scheduler;SolarConfig config;std::vector<Event> events;
 void begin(){assert(scheduler.begin(config));}
 void tick(int64_t utc,bool ready=true){scheduler.tick(utc,ready,config,[&](bool dawn,const SolarEntry& entry,int64_t eventUtc){events.push_back({dawn,entry,eventUtc});});}
 void run(int64_t utc){tick(utc-60);tick(utc);}
};
static void testValidation(){
 SolarConfig config;assert(validSolarConfig(config));assert(!config.dawn.enabled&&!config.dusk.enabled);
 assert(config.latitudeE6==10776000&&config.longitudeE6==106700000);
 assert(config.dawn.days==127&&config.dawn.offset==0&&config.dawn.minutes==30&&config.dawn.favorite==-1&&config.dawn.brightness==35);
 SolarEntry entry;entry.days=0;assert(validSolarEntry(entry));entry.enabled=true;assert(!validSolarEntry(entry));entry.days=127;
 entry.days=128;assert(!validSolarEntry(entry));entry.days=127;
 entry.offset=-120;assert(validSolarEntry(entry));entry.offset=120;assert(validSolarEntry(entry));entry.offset=-121;assert(!validSolarEntry(entry));entry.offset=121;assert(!validSolarEntry(entry));entry.offset=0;
 entry.minutes=1;assert(validSolarEntry(entry));entry.minutes=120;assert(validSolarEntry(entry));entry.minutes=0;assert(!validSolarEntry(entry));entry.minutes=121;assert(!validSolarEntry(entry));entry.minutes=30;
 for(int i=-1;i<=2;i++){entry.favorite=i;assert(validSolarEntry(entry));}entry.favorite=-2;assert(!validSolarEntry(entry));entry.favorite=3;assert(!validSolarEntry(entry));entry.favorite=-1;
 entry.brightness=1;assert(validSolarEntry(entry));entry.brightness=100;assert(validSolarEntry(entry));entry.brightness=0;assert(!validSolarEntry(entry));entry.brightness=101;assert(!validSolarEntry(entry));
 config.latitudeE6=89000000;config.longitudeE6=180000000;assert(validSolarConfig(config));config.latitudeE6=-89000000;config.longitudeE6=-180000000;assert(validSolarConfig(config));
 config.latitudeE6=89000001;assert(!validSolarConfig(config));config.latitudeE6=-89000001;assert(!validSolarConfig(config));config=SolarConfig();config.longitudeE6=180000001;assert(!validSolarConfig(config));config.longitudeE6=-180000001;assert(!validSolarConfig(config));
 config=SolarConfig();SolarConfig other=config;assert(other==config);other.dawn.minutes++;assert(other!=config);other=config;other.latitudeE6++;assert(other!=config);other=config;other.dusk.brightness++;assert(other!=config);
 config.dusk.favorite=3;SolarTimes rejected;assert(!calculateSolarTimes(MONDAY,config,rejected)&&!rejected.sunriseValid&&!rejected.sunsetValid);
 SolarScheduler invalid;assert(!invalid.begin(config));config=SolarConfig();assert(!calculateSolarTimes(INT64_MAX,config,rejected));assert(!calculateSolarTimes(INT64_MIN,config,rejected));
}
static void testIndependentReferencesAndRounding(){
 // Independent fixtures: PyEphem 4.2.1 / XEphem libastro, sea level,
 // pressure=0, solar-center horizon -0.833 degrees. They do not use the
 // NOAA equations in Solar.h. Allow two minutes for model differences.
 SolarConfig config;SolarTimes hcm=timesFor(MONDAY,config);
 assert(hcm.sunriseValid&&hcm.sunsetValid);
 assert(std::llabs(hcm.sunriseUtc-1791758520LL)<=120); // 05:42:00 ICT.
 assert(std::llabs(hcm.sunsetUtc-1791801443LL)<=120); // 17:37:23 ICT.
 assert(hcm.sunriseUtc%60==0&&hcm.sunsetUtc%60==0);
 ScheduleWallTime rise,set;assert(localScheduleTime(hcm.sunriseUtc,rise)&&localScheduleTime(hcm.sunsetUtc,set));
 assert(rise.localDay==MONDAY&&set.localDay==MONDAY&&rise.weekday==0&&set.weekday==0);

 // The classic Wayne, NJ location also checks a different date/hemisphere
 // and UTC date rollover. June 25 EDT sunset is June 26 00:33 UTC, hence
 // belongs to June 26 when this product's fixed local clock is UTC+7.
 config.latitudeE6=40900000;config.longitudeE6=-74300000;
 SolarTimes njRise=timesFor(7480,config),njSet=timesFor(7481,config);
 assert(njRise.sunriseValid&&njSet.sunsetValid);
 assert(std::llabs(njRise.sunriseUtc-646305991LL)<=120);
 assert(std::llabs(njSet.sunsetUtc-646360380LL)<=120);

 config.latitudeE6=0;config.longitudeE6=179900000;SolarTimes east=timesFor(MONDAY,config);
 assert(std::llabs(east.sunriseUtc-1791740624LL)<=120);
 config.longitudeE6=-179900000;SolarTimes west=timesFor(MONDAY,config);
 assert(std::llabs(west.sunriseUtc-1791740576LL)<=120);
 assert(std::llabs(east.sunriseUtc-west.sunriseUtc)<120); // Neither IDL side is a day late.
 assert(localScheduleTime(east.sunriseUtc,rise)&&rise.localDay==MONDAY);
 assert(localScheduleTime(west.sunriseUtc,rise)&&rise.localDay==MONDAY);
 config.longitudeE6=15000000;SolarTimes late=timesFor(MONDAY,config);
 assert(std::llabs(late.sunsetUtc-1791823787LL)<=120&&localScheduleTime(late.sunsetUtc,set)&&set.localDay==MONDAY);
}
static void testPolarDatesAndLongClock(){
 SolarConfig config;config.latitudeE6=69650000;config.longitudeE6=18950000; // Tromso.
 SolarTimes summer=timesFor(20625,config),winter=timesFor(20808,config);
 assert(!summer.sunriseValid&&!summer.sunsetValid&&!winter.sunriseValid&&!winter.sunsetValid);
 config.latitudeE6=-80000000;summer=timesFor(20625,config);winter=timesFor(20808,config);
 assert(!summer.sunriseValid&&!summer.sunsetValid&&!winter.sunriseValid&&!winter.sunsetValid);
 config=SolarConfig();
 SolarTimes leap=timesFor(11016,config),leapNext=timesFor(11017,config); // 2000-02-29 / 03-01.
 SolarTimes century=timesFor(-25508,config); // 1900-03-01; 1900 was not a leap year.
 assert(leap.sunriseValid&&leapNext.sunriseValid&&century.sunriseValid);
 assert(leapNext.sunriseUtc-leap.sunriseUtc>86000&&leapNext.sunriseUtc-leap.sunriseUtc<86800);
 SolarTimes future=timesFor(47401,config); // 2099-10-12, safely beyond signed 32-bit UTC.
 assert(future.sunriseValid&&future.sunsetValid&&future.sunriseUtc>2147483647LL);
 ScheduleWallTime wall;assert(localScheduleTime(future.sunriseUtc,wall)&&wall.localDay==47401);
 Fixture fixture;fixture.config.dawn.enabled=true;fixture.begin();fixture.run(future.sunriseUtc);assert(fixture.events.size()==1);
 const SolarTimes& last=fixture.scheduler.today(SCHEDULE_MAX_UTC,fixture.config);
 assert(last.localDay==47482&&last.sunriseValid); // UTC+7 has entered 2100-01-01.
 assert(!fixture.scheduler.today(SCHEDULE_MAX_UTC+1,fixture.config).sunriseValid);
}
static void testWeekdaysAndSourceDateOffsets(){
 for(int selected=0;selected<7;selected++)for(int day=0;day<7;day++){
  Fixture fixture;fixture.config.dawn.enabled=true;fixture.config.dawn.days=uint8_t(1u<<selected);fixture.config.dawn.favorite=2;fixture.config.dawn.minutes=45;fixture.begin();
  int64_t event=due(MONDAY+day,fixture.config);fixture.run(event);
  assert(fixture.events.size()==unsigned(selected==day));
  if(!fixture.events.empty())assert(fixture.events[0].dawn&&fixture.events[0].entry.favorite==2&&fixture.events[0].entry.minutes==45&&fixture.events[0].utc==event);
 }
 Fixture early;early.config.latitudeE6=0;early.config.longitudeE6=179900000;early.config.dawn.enabled=true;early.config.dawn.days=1;early.config.dawn.offset=-60;early.begin();
 int64_t earlyUtc=due(MONDAY,early.config);ScheduleWallTime wall;assert(localScheduleTime(earlyUtc,wall)&&wall.localDay==MONDAY-1&&wall.weekday==6);
 early.run(earlyUtc);assert(early.events.size()==1&&early.events[0].utc==earlyUtc);
 Fixture wrongDay;wrongDay.config=early.config;wrongDay.config.dawn.days=64;wrongDay.begin();wrongDay.run(earlyUtc);assert(wrongDay.events.empty());
 early.tick(earlyUtc+60);early.tick(earlyUtc-60);early.tick(earlyUtc);assert(early.events.size()==1);

 Fixture late;late.config.latitudeE6=0;late.config.longitudeE6=15000000;late.config.dusk.enabled=true;late.config.dusk.days=1;late.config.dusk.offset=60;late.begin();
 int64_t lateUtc=due(MONDAY,late.config,false);assert(localScheduleTime(lateUtc,wall)&&wall.localDay==MONDAY+1&&wall.weekday==1);
 late.run(lateUtc);assert(late.events.size()==1&&!late.events[0].dawn&&late.events[0].utc==lateUtc);
 late.tick(lateUtc+60);late.tick(lateUtc-60);late.tick(lateUtc);assert(late.events.size()==1);
}
static void testFreshClockNoCatchupAndDuplicateGuards(){
 Fixture fixture;fixture.config.dawn.enabled=true;fixture.begin();int64_t first=due(MONDAY,fixture.config);
 fixture.tick(first-60,false);fixture.tick(first,false);fixture.tick(INT64_MAX);fixture.tick(SCHEDULE_MIN_UTC-1);assert(fixture.events.empty());
 fixture.tick(first);fixture.tick(first+30);assert(fixture.events.empty()); // First ready minute only arms.
 int64_t second=due(MONDAY+1,fixture.config);fixture.run(second);assert(fixture.events.size()==1);
 for(int i=0;i<60;i++)fixture.tick(second+i);
 assert(fixture.events.size()==1);
 fixture.tick(second+60);fixture.run(second);fixture.run(first);assert(fixture.events.size()==1); // Backward corrections cannot replay an occurrence.
 int64_t third=due(MONDAY+2,fixture.config);fixture.run(third);assert(fixture.events.size()==2);

 Fixture jump;jump.config.dusk.enabled=true;jump.begin();int64_t sunset=due(MONDAY,jump.config,false);
 jump.tick(sunset-60);jump.tick(sunset+300);jump.tick(sunset+360);assert(jump.events.empty());
 int64_t next=due(MONDAY+1,jump.config,false);jump.tick(next);assert(jump.events.empty()); // A jump into a due minute primes it.
 jump.run(sunset);assert(jump.events.size()==1); // Skipped future observation did not mark its date.
 jump.scheduler.rebaseline(next,true);jump.tick(next+20);assert(jump.events.size()==1);
 jump.run(next);assert(jump.events.size()==2);
 jump.scheduler.rebaseline(next-60,true);jump.tick(next);assert(jump.events.size()==2);

 Fixture disabled;disabled.begin();disabled.run(first);assert(disabled.events.empty());
 Fixture polar;polar.config.dawn.enabled=true;polar.config.dusk.enabled=true;polar.config.latitudeE6=89000000;polar.begin();
 for(int i=0;i<1440;i++)polar.tick(20625*86400LL-SCHEDULE_UTC_OFFSET+i*60);
 assert(polar.events.empty());
}
static void testEditsLocationAndCallbackGuard(){
 Fixture fixture;fixture.config.dawn.enabled=true;fixture.begin();int64_t sunrise=due(MONDAY,fixture.config);fixture.run(sunrise);assert(fixture.events.size()==1);
 assert(fixture.scheduler.configure(fixture.config,sunrise-60,true));fixture.tick(sunrise);assert(fixture.events.size()==1); // Unchanged saves preserve the guard.
 fixture.config.dawn.offset=1;fixture.config.dawn.brightness=20;assert(fixture.scheduler.configure(fixture.config,sunrise,true));fixture.tick(sunrise+60);
 assert(fixture.events.size()==2&&fixture.events.back().entry.brightness==20);
 SolarConfig invalid=fixture.config;invalid.dawn.minutes=0;assert(!fixture.scheduler.configure(invalid,sunrise+60,true));
 fixture.config.longitudeE6+=100000;assert(fixture.scheduler.configure(fixture.config,sunrise+60,true));
 fixture.tick(sunrise+60);assert(fixture.events.size()==2); // A location edit also primes its current minute.
 int64_t next=due(MONDAY+1,fixture.config);fixture.run(next);assert(fixture.events.size()==3);

 Fixture automatic;automatic.config.dawn.enabled=true;automatic.begin();automatic.tick(sunrise-60);
 automatic.config.dawn.brightness=70;automatic.tick(sunrise);automatic.tick(sunrise+20);assert(automatic.events.empty());
 automatic.config.dawn.offset=1;automatic.tick(sunrise+30);automatic.tick(sunrise+60);assert(automatic.events.size()==1);

 SolarScheduler scheduler;SolarConfig config;config.dawn.enabled=true;assert(scheduler.begin(config));unsigned count=0;
 auto increment=[&](bool,const SolarEntry&,int64_t){count++;};scheduler.tick(sunrise-60,true,config,increment);
 scheduler.tick(sunrise,true,config,[&](bool,const SolarEntry&,int64_t){count++;scheduler.tick(sunrise,true,config,increment);});
 assert(count==1);assert(scheduler.begin(config));scheduler.tick(sunrise,true,config,increment);assert(count==1);
}
int main(){
 testValidation();testIndependentReferencesAndRounding();testPolarDatesAndLongClock();testWeekdaysAndSourceDateOffsets();testFreshClockNoCatchupAndDuplicateGuards();testEditsLocationAndCallbackGuard();
 std::cout<<"PASS: independent astronomical references, UTC+7/IDL dates, polar absence, 64-bit clock, source-weekday offsets, fresh-clock/edit arming, no catch-up and duplicate guards\n";
}
