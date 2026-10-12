#pragma once
#include <stdint.h>

namespace Deco {
constexpr uint8_t SCHEDULE_SLOTS=8;
constexpr int32_t SCHEDULE_UTC_OFFSET=7*60*60;
constexpr int64_t SCHEDULE_MIN_UTC=1704067200LL; // 2024-01-01 UTC
constexpr int64_t SCHEDULE_MAX_UTC=4102444799LL; // Last second of 2099 UTC.

struct ScheduleEntry {
 bool enabled=false;
 uint8_t days=0; // Monday bit 0 through Sunday bit 6, in Vietnam time.
 uint16_t minute=0;
 bool power=false;
 int8_t favorite=-1;
 bool operator==(const ScheduleEntry& other)const{
  return enabled==other.enabled&&days==other.days&&minute==other.minute&&power==other.power&&favorite==other.favorite;
 }
 bool operator!=(const ScheduleEntry& other)const{return !(*this==other);}
};
struct ScheduleConfig {
 bool enabled=false;
 ScheduleEntry entries[SCHEDULE_SLOTS];
 bool operator==(const ScheduleConfig& other)const{
  if(enabled!=other.enabled)return false;
  for(uint8_t i=0;i<SCHEDULE_SLOTS;i++)if(entries[i]!=other.entries[i])return false;
  return true;
 }
 bool operator!=(const ScheduleConfig& other)const{return !(*this==other);}
};
inline bool validScheduleEntry(const ScheduleEntry& entry){
 return entry.days<=127&&(!entry.enabled||entry.days!=0)&&entry.minute<1440&&
        (entry.power?(entry.favorite>=0&&entry.favorite<=2):entry.favorite==-1);
}
inline bool validScheduleConfig(const ScheduleConfig& config){
 for(const auto& entry:config.entries)if(!validScheduleEntry(entry))return false;
 return true;
}

struct ScheduleWallTime {
 int64_t localDay=0;
 int64_t epochMinute=0;
 uint8_t weekday=0;
 uint16_t minute=0;
};
inline bool localScheduleTime(int64_t utc,ScheduleWallTime& wall){
 if(utc<SCHEDULE_MIN_UTC||utc>SCHEDULE_MAX_UTC)return false;
 int64_t local=utc+SCHEDULE_UTC_OFFSET;
 ScheduleWallTime result;
 result.localDay=local/86400;
 result.epochMinute=utc/60;
 result.weekday=uint8_t((result.localDay+3)%7); // Unix day 0 was Thursday.
 result.minute=uint16_t((local%86400)/60);
 wall=result;return true;
}

class Scheduler {
 ScheduleConfig known;
 bool configured=false,haveMinute=false;
 int64_t evaluatedMinute=0;
 bool fired[SCHEDULE_SLOTS]={};
 int64_t lastFireDay[SCHEDULE_SLOTS]={};
 public:
 bool begin(const ScheduleConfig& config){
  configured=false;haveMinute=false;
  for(uint8_t i=0;i<SCHEDULE_SLOTS;i++){fired[i]=false;lastFireDay[i]=0;}
  if(!validScheduleConfig(config))return false;
  known=config;configured=true;return true;
 }
 // Use on an actual clock step or OTA resume, not on every periodic NTP
 // callback. Already-fired entries remain guarded across a clock rebaseline.
 void rebaseline(int64_t utc,bool clockReady){
  ScheduleWallTime wall;haveMinute=clockReady&&localScheduleTime(utc,wall);
  if(haveMinute)evaluatedMinute=wall.epochMinute;
 }
 bool configure(const ScheduleConfig& config,int64_t utc,bool clockReady){
  if(!validScheduleConfig(config))return false;
  for(uint8_t i=0;i<SCHEDULE_SLOTS;i++){
   // Fixed slots keep their date guards through master switches and
   // unchanged saves; a newly edited action can run later on the same day.
   if(!configured||config.entries[i]!=known.entries[i]){fired[i]=false;lastFireDay[i]=0;}
  }
  known=config;configured=true;rebaseline(utc,clockReady);return true;
 }
 template<class Emit> void tick(int64_t utc,bool clockReady,const ScheduleConfig& config,Emit emit){
  if(!validScheduleConfig(config))return;
  if(!configured){if(!begin(config))return;}
  else if(config!=known){configure(config,utc,clockReady);return;}
  ScheduleWallTime wall;
  if(!clockReady||!localScheduleTime(utc,wall))return;
  if(!haveMinute){haveMinute=true;evaluatedMinute=wall.epochMinute;return;}
  int64_t delta=wall.epochMinute-evaluatedMinute;
  if(delta==0)return;
  evaluatedMinute=wall.epochMinute;
  // A discontinuity primes the new minute rather than replaying missed
  // actions. It also avoids acting at a transient future NTP timestamp.
  if(delta<0||delta>1||!config.enabled)return;
  uint8_t today=uint8_t(1u<<wall.weekday);
  // Emit ON first and OFF last so an OFF action wins a simultaneous clash.
  for(uint8_t pass=0;pass<2;pass++)for(uint8_t i=0;i<SCHEDULE_SLOTS;i++){
   const auto& entry=config.entries[i];
   if(entry.power!=(pass==0)||!entry.enabled||!(entry.days&today)||entry.minute!=wall.minute)continue;
   if(fired[i]&&wall.localDay<=lastFireDay[i])continue;
   // Mark before invoking the caller, including when it cannot apply an ON
   // because the selected favorite is empty. No per-minute NVS writes.
   fired[i]=true;lastFireDay[i]=wall.localDay;emit(i,entry);
  }
 }
};
}
