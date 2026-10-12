#include "../DenDeco/Schedule.h"
#include <cassert>
#include <iostream>
#include <vector>
using namespace Deco;

static const int64_t MONDAY_0700=1791763200LL; // 2026-10-12 00:00 UTC
static const int64_t WEEK=7*86400LL;
static ScheduleEntry on(uint16_t minute,uint8_t days=127,int8_t favorite=0){
 ScheduleEntry entry;entry.enabled=true;entry.days=days;entry.minute=minute;entry.power=true;entry.favorite=favorite;return entry;
}
static ScheduleEntry off(uint16_t minute,uint8_t days=127){
 ScheduleEntry entry;entry.enabled=true;entry.days=days;entry.minute=minute;return entry;
}
struct Event {uint8_t index;bool power;int8_t favorite;};
struct Fixture {
 Scheduler scheduler;ScheduleConfig config;std::vector<Event> events;
 void begin(){assert(scheduler.begin(config));}
 void tick(int64_t utc,bool ready=true){scheduler.tick(utc,ready,config,[&](uint8_t index,const ScheduleEntry& entry){events.push_back({index,entry.power,entry.favorite});});}
};
static void testValidation(){
 ScheduleConfig defaults;assert(!defaults.enabled&&validScheduleConfig(defaults));
 for(auto& entry:defaults.entries)assert(!entry.enabled&&!entry.power&&entry.favorite==-1);
 ScheduleEntry entry;assert(validScheduleEntry(entry));entry.enabled=true;assert(!validScheduleEntry(entry));
 entry.days=1;assert(validScheduleEntry(entry));entry.days=128;assert(!validScheduleEntry(entry));
 entry.days=127;entry.minute=1439;assert(validScheduleEntry(entry));entry.minute=1440;assert(!validScheduleEntry(entry));
 entry.minute=0;entry.favorite=0;assert(!validScheduleEntry(entry));entry.power=true;
 for(int favorite=0;favorite<3;favorite++){entry.favorite=favorite;assert(validScheduleEntry(entry));}
 entry.favorite=3;assert(!validScheduleEntry(entry));entry.favorite=-1;assert(!validScheduleEntry(entry));
 entry.enabled=false;assert(!validScheduleEntry(entry)); // ON always needs a favorite.
 entry.power=false;entry.days=0;assert(validScheduleEntry(entry));
 defaults.entries[7]=entry;assert(validScheduleConfig(defaults));defaults.entries[7].minute=65535;assert(!validScheduleConfig(defaults));
 Scheduler invalid;assert(!invalid.begin(defaults));
}
static void testVietnamClock(){
 ScheduleWallTime sunday,monday,utcMorning;
 assert(localScheduleTime(1791737940LL,sunday)&&sunday.weekday==6&&sunday.minute==1439);
 assert(localScheduleTime(1791738000LL,monday)&&monday.weekday==0&&monday.minute==0&&monday.localDay==sunday.localDay+1);
 assert(localScheduleTime(MONDAY_0700,utcMorning)&&utcMorning.weekday==0&&utcMorning.minute==420&&utcMorning.localDay==monday.localDay);
 ScheduleWallTime endYear,newYear;
 assert(localScheduleTime(1798736340LL,endYear)&&endYear.weekday==3&&endYear.minute==1439);
 assert(localScheduleTime(1798736400LL,newYear)&&newYear.weekday==4&&newYear.minute==0&&newYear.localDay==endYear.localDay+1);
 ScheduleWallTime boundary;
 assert(localScheduleTime(SCHEDULE_MIN_UTC,boundary)&&boundary.weekday==0&&boundary.minute==420);
 const int64_t savedDay=boundary.localDay;
 assert(!localScheduleTime(SCHEDULE_MIN_UTC-1,boundary)&&boundary.localDay==savedDay);
 assert(!localScheduleTime(INT64_MIN,boundary)&&!localScheduleTime(INT64_MAX,boundary));
 assert(localScheduleTime(SCHEDULE_MAX_UTC,boundary)&&boundary.weekday==4&&boundary.minute==419);
 assert(!localScheduleTime(SCHEDULE_MAX_UTC+1,boundary));
}
static void testWeekdaysAndMidnight(){
 for(int selected=0;selected<7;selected++)for(int today=0;today<7;today++){
  Fixture fixture;fixture.config.enabled=true;fixture.config.entries[0]=on(420,uint8_t(1u<<selected),2);fixture.begin();
  int64_t utc=MONDAY_0700+today*86400LL;fixture.tick(utc-60);fixture.tick(utc);
  assert(fixture.events.size()==unsigned(selected==today));
  if(!fixture.events.empty())assert(fixture.events[0].index==0&&fixture.events[0].power&&fixture.events[0].favorite==2);
 }
 Fixture midnight;midnight.config.enabled=true;midnight.config.entries[0]=off(1439,64);midnight.config.entries[1]=on(0,1,1);midnight.begin();
 midnight.tick(1791737880LL);midnight.tick(1791737940LL);midnight.tick(1791738000LL);
 assert(midnight.events.size()==2&&!midnight.events[0].power&&midnight.events[1].power&&midnight.events[1].index==1);
}
static void testReadinessAndMinuteGuards(){
 Fixture fixture;fixture.config.enabled=true;fixture.config.entries[0]=on(420);fixture.begin();
 fixture.tick(MONDAY_0700-60,false);fixture.tick(MONDAY_0700,false);assert(fixture.events.empty());
 fixture.tick(MONDAY_0700);fixture.tick(MONDAY_0700+30);assert(fixture.events.empty()); // Fresh boot arms next minute.
 fixture.tick(MONDAY_0700+WEEK-60);fixture.tick(MONDAY_0700+WEEK);assert(fixture.events.size()==1);
 for(int seconds=0;seconds<60;seconds++)fixture.tick(MONDAY_0700+WEEK+seconds);
 assert(fixture.events.size()==1);
 fixture.tick(MONDAY_0700+WEEK+60);
 fixture.tick(MONDAY_0700+WEEK-120);fixture.tick(MONDAY_0700+WEEK-60);fixture.tick(MONDAY_0700+WEEK);
 assert(fixture.events.size()==1); // A backward correction cannot replay today's event.
 fixture.tick(MONDAY_0700+2*WEEK-60);fixture.tick(MONDAY_0700+2*WEEK);assert(fixture.events.size()==2);
 fixture.tick(MONDAY_0700+WEEK-60);fixture.tick(MONDAY_0700+WEEK);assert(fixture.events.size()==2); // Older date cannot replay either.

 Fixture invalid;invalid.config.enabled=true;invalid.config.entries[0]=on(420);invalid.begin();
 invalid.tick(SCHEDULE_MIN_UTC-1);invalid.tick(SCHEDULE_MAX_UTC+1);invalid.tick(INT64_MAX);invalid.tick(MONDAY_0700,false);assert(invalid.events.empty());
 invalid.tick(MONDAY_0700-60);invalid.tick(MONDAY_0700);assert(invalid.events.size()==1);
 Fixture disabled;disabled.config.entries[0]=on(420);disabled.begin();disabled.tick(MONDAY_0700-60);disabled.tick(MONDAY_0700);assert(disabled.events.empty());
}
static void testClockJumpsAndNoCatchup(){
 Fixture fixture;fixture.config.enabled=true;fixture.config.entries[0]=on(420);fixture.begin();
 fixture.tick(MONDAY_0700-60);fixture.tick(MONDAY_0700+5*60);fixture.tick(MONDAY_0700+6*60);
 assert(fixture.events.empty()); // Skipping a due minute does not run it late.
 fixture.tick(MONDAY_0700+WEEK);assert(fixture.events.empty()); // Prime an apparent future due minute.
 fixture.tick(MONDAY_0700-60);fixture.tick(MONDAY_0700);assert(fixture.events.size()==1); // Future observation did not poison its guard.
 fixture.tick(MONDAY_0700+WEEK-60);fixture.tick(MONDAY_0700+WEEK);assert(fixture.events.size()==2);
 fixture.tick(MONDAY_0700-60);fixture.tick(MONDAY_0700);assert(fixture.events.size()==2);

 // An explicit OTA/actual-clock-step rebaseline suppresses its current
 // minute and preserves the guards; ordinary ticks need no NTP callback.
 Fixture rebaseline;rebaseline.config.enabled=true;rebaseline.config.entries[0]=on(420);rebaseline.begin();
 rebaseline.tick(MONDAY_0700-60);rebaseline.scheduler.rebaseline(MONDAY_0700,true);rebaseline.tick(MONDAY_0700+10);
 assert(rebaseline.events.empty());rebaseline.tick(MONDAY_0700+WEEK-60);rebaseline.tick(MONDAY_0700+WEEK);
 assert(rebaseline.events.size()==1);rebaseline.scheduler.rebaseline(MONDAY_0700+WEEK-60,true);rebaseline.tick(MONDAY_0700+WEEK);
 assert(rebaseline.events.size()==1);
}
static void testConfigurationArming(){
 Fixture enable;enable.config.entries[0]=on(420);enable.begin();enable.tick(MONDAY_0700-60);
 enable.config.enabled=true;assert(enable.scheduler.configure(enable.config,MONDAY_0700,true));enable.tick(MONDAY_0700+10);enable.tick(MONDAY_0700+60);
 assert(enable.events.empty());enable.tick(MONDAY_0700+WEEK-60);enable.tick(MONDAY_0700+WEEK);assert(enable.events.size()==1);

 Fixture edit;edit.config.enabled=true;edit.config.entries[0]=on(420);edit.begin();edit.tick(MONDAY_0700-60);edit.tick(MONDAY_0700);
 assert(edit.events.size()==1);assert(edit.scheduler.configure(edit.config,MONDAY_0700+10,true));
 edit.config.entries[0]=on(421,127,2);assert(edit.scheduler.configure(edit.config,MONDAY_0700+20,true));
 edit.tick(MONDAY_0700+30);assert(edit.events.size()==1);edit.tick(MONDAY_0700+60);
 assert(edit.events.size()==2&&edit.events.back().favorite==2);
 // Unchanged slots retain their guard through a master off/on toggle.
 edit.config.enabled=false;assert(edit.scheduler.configure(edit.config,MONDAY_0700+120,true));
 edit.config.enabled=true;assert(edit.scheduler.configure(edit.config,MONDAY_0700+180,true));
 edit.tick(MONDAY_0700);edit.tick(MONDAY_0700+60);assert(edit.events.size()==2);

 // A caller forgetting configure still cannot fire an edit immediately.
 Fixture automatic;automatic.config.enabled=true;automatic.config.entries[0]=on(419);automatic.begin();automatic.tick(MONDAY_0700-60);
 automatic.config.entries[0]=on(420);automatic.tick(MONDAY_0700);automatic.tick(MONDAY_0700+1);assert(automatic.events.empty());
 automatic.config.entries[0]=on(421);automatic.tick(MONDAY_0700+30);automatic.tick(MONDAY_0700+60);assert(automatic.events.size()==1);

 Fixture bad;bad.config.enabled=true;bad.config.entries[0]=on(420);bad.begin();bad.tick(MONDAY_0700-60);
 ScheduleConfig rejected=bad.config;rejected.entries[0].favorite=-1;assert(!bad.scheduler.configure(rejected,MONDAY_0700,true));
 bad.tick(MONDAY_0700);assert(bad.events.size()==1); // Rejected edits leave runtime intact.
}
static void testCollisionsAndCallbackGuard(){
 Fixture clash;clash.config.enabled=true;
 for(int i=0;i<SCHEDULE_SLOTS;i++)clash.config.entries[i]=(i==1||i==3||i==4||i==7)?on(420,127,i%3):off(420);
 clash.begin();clash.tick(MONDAY_0700-60);clash.tick(MONDAY_0700);clash.tick(MONDAY_0700+1);
 const uint8_t expected[8]={1,3,4,7,0,2,5,6};assert(clash.events.size()==8);
 for(int i=0;i<8;i++){assert(clash.events[i].index==expected[i]);assert(clash.events[i].power==(i<4));}
 assert(!clash.events.back().power); // Deterministic OFF wins when caller applies in order.
 Scheduler scheduler;ScheduleConfig config;config.enabled=true;config.entries[0]=on(420);assert(scheduler.begin(config));unsigned calls=0;
 auto count=[&](uint8_t,const ScheduleEntry&){calls++;};scheduler.tick(MONDAY_0700-60,true,config,count);
 scheduler.tick(MONDAY_0700,true,config,[&](uint8_t,const ScheduleEntry&){calls++;scheduler.tick(MONDAY_0700,true,config,count);});
 scheduler.tick(MONDAY_0700+1,true,config,count);assert(calls==1);
 // A new boot clears RAM guards but still requires a first fresh-clock
 // priming minute; no claim of persistent occurrence history is made.
 assert(scheduler.begin(config));scheduler.tick(MONDAY_0700,true,config,count);assert(calls==1);
}
int main(){
 testValidation();testVietnamClock();testWeekdaysAndMidnight();testReadinessAndMinuteGuards();testClockJumpsAndNoCatchup();testConfigurationArming();testCollisionsAndCallbackGuard();
 std::cout<<"PASS: weekly UTC+7 weekdays/midnight, favorite validation, fresh-clock/edit arming, no late catchup, duplicate/backward guards, deterministic OFF precedence and date bounds\n";
}
