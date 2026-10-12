#pragma once
#include <stdint.h>
#include <math.h>
#include "Schedule.h"

namespace Deco {

struct SolarEntry {
 bool enabled=false;
 uint8_t days=127; // Monday bit 0 through Sunday bit 6, on the source date.
 int16_t offset=0; // Minutes relative to sunrise/sunset, including date rollover.
 uint16_t minutes=30; // Dawn ramp duration; unused by dusk.
 int8_t favorite=-1; // -1: built-in warm target; 0..2: live favorite.
 uint8_t brightness=35;
 bool operator==(const SolarEntry& other)const{
  return enabled==other.enabled&&days==other.days&&offset==other.offset&&
         minutes==other.minutes&&favorite==other.favorite&&brightness==other.brightness;
 }
 bool operator!=(const SolarEntry& other)const{return !(*this==other);}
};
struct SolarConfig {
 int32_t latitudeE6=10776000,longitudeE6=106700000;
 SolarEntry dawn,dusk;
 bool operator==(const SolarConfig& other)const{
  return latitudeE6==other.latitudeE6&&longitudeE6==other.longitudeE6&&dawn==other.dawn&&dusk==other.dusk;
 }
 bool operator!=(const SolarConfig& other)const{return !(*this==other);}
};
inline bool validSolarEntry(const SolarEntry& entry){
 return entry.days<=127&&(!entry.enabled||entry.days!=0)&&entry.offset>=-120&&entry.offset<=120&&
        entry.minutes>=1&&entry.minutes<=120&&entry.favorite>=-1&&entry.favorite<=2&&
        entry.brightness>=1&&entry.brightness<=100;
}
inline bool validSolarConfig(const SolarConfig& config){
 return config.latitudeE6>=-89000000&&config.latitudeE6<=89000000&&
        config.longitudeE6>=-180000000&&config.longitudeE6<=180000000&&
        validSolarEntry(config.dawn)&&validSolarEntry(config.dusk);
}
struct SolarTimes {
 bool sunriseValid=false,sunsetValid=false;
 int64_t sunriseUtc=0,sunsetUtc=0,localDay=0;
};

namespace SolarDetail {
constexpr double SOLAR_PI=3.14159265358979323846;
inline double toRadians(double value){return value*(SOLAR_PI/180.0);}
inline double toDegrees(double value){return value*(180.0/SOLAR_PI);}
inline double wrap(double value,double period){double result=fmod(value,period);return result<0?result+period:result;}

// NOAA solar-position equations. Unix day -> Julian date keeps the actual
// Gregorian date (including leap centuries), rather than reusing an annual
// day-of-year approximation. Height is sea level; zenith 90.833 degrees
// includes the conventional solar radius and atmospheric refraction.
inline double altitudeDifference(double utc,double latitude,double longitude){
 double t=(2440587.5+utc/86400.0-2451545.0)/36525.0;
 double l=toRadians(wrap(280.46646+t*(36000.76983+t*0.0003032),360.0));
 double m=toRadians(wrap(357.52911+t*(35999.05029-0.0001537*t),360.0));
 double e=0.016708634-t*(0.000042037+0.0000001267*t);
 double center=sin(m)*(1.914602-t*(0.004817+0.000014*t))+
               sin(2*m)*(0.019993-0.000101*t)+sin(3*m)*0.000289;
 double omega=toRadians(125.04-1934.136*t);
 double apparent=l+toRadians(center-0.00569-0.00478*sin(omega));
 double obliquity=toRadians(23.0+(26.0+(21.448-t*(46.815+t*(0.00059-t*0.001813)))/60.0)/60.0+
                          0.00256*cos(omega));
 double declination=asin(sin(obliquity)*sin(apparent));
 double y=tan(obliquity/2);y*=y;
 double equation=4*toDegrees(y*sin(2*l)-2*e*sin(m)+4*e*y*sin(m)*cos(2*l)-
                           0.5*y*y*sin(4*l)-1.25*e*e*sin(2*m));
 double solarMinute=wrap(wrap(utc/60.0,1440.0)+equation+4*longitude,1440.0);
 double angle=toRadians(solarMinute/4-180);
 return sin(latitude)*sin(declination)+cos(latitude)*cos(declination)*cos(angle)-cos(toRadians(90.833));
}
inline bool opposite(double a,double b){return (a<0&&b>=0)||(a>=0&&b<0);}
inline double crossing(double left,double right,double a,double latitude,double longitude){
 // 26 subdivisions of an hour provide sub-millisecond root precision.
 for(uint8_t i=0;i<26;i++){
  double middle=(left+right)/2,b=altitudeDifference(middle,latitude,longitude);
  if(opposite(a,b)){right=middle;}else{left=middle;a=b;}
 }
 return (left+right)/2;
}
inline double extremum(double left,double right,bool maximum,double latitude,double longitude){
 const double ratio=0.6180339887498948482;
 double direction=maximum?1.0:-1.0;
 double a=right-ratio*(right-left),b=left+ratio*(right-left);
 double fa=direction*altitudeDifference(a,latitude,longitude),fb=direction*altitudeDifference(b,latitude,longitude);
 for(uint8_t i=0;i<32;i++){
  if(fa<fb){left=a;a=b;fa=fb;b=left+ratio*(right-left);fb=direction*altitudeDifference(b,latitude,longitude);}
  else{right=b;b=a;fb=fa;a=right-ratio*(right-left);fa=direction*altitudeDifference(a,latitude,longitude);}
 }
 return (left+right)/2;
}
inline void recordCrossing(double utc,double begin,double end,double latitude,double longitude,SolarTimes& times){
 if(utc<begin||utc>=end)return;
 bool rising=altitudeDifference(utc+1,latitude,longitude)>altitudeDifference(utc-1,latitude,longitude);
 // Round astronomical events once, before applying an integer minute offset.
 int64_t rounded=int64_t(floor((utc+30.0)/60.0))*60;
 if(rising){if(!times.sunriseValid||rounded<times.sunriseUtc){times.sunriseValid=true;times.sunriseUtc=rounded;}}
 else if(!times.sunsetValid||rounded<times.sunsetUtc){times.sunsetValid=true;times.sunsetUtc=rounded;}
}
inline void inspectInterval(double left,double right,double a,double b,double begin,double end,
                            double latitude,double longitude,SolarTimes& times){
 if(opposite(a,b))recordCrossing(crossing(left,right,a,latitude,longitude),begin,end,latitude,longitude,times);
}
} // namespace SolarDetail

inline bool calculateSolarTimes(int64_t localDay,const SolarConfig& config,SolarTimes& output){
 SolarTimes result;result.localDay=localDay;output=result;
 // Gregorian 0001-01-01..9999-12-31 protects arithmetic. The scheduling
 // clock itself is limited to the project's supported 2024..2099 UTC range.
 if(!validSolarConfig(config)||localDay<-719162LL||localDay>2932896LL)return false;
 const double begin=double(localDay*86400LL-SCHEDULE_UTC_OFFSET),end=begin+86400.0;
 const double latitude=SolarDetail::toRadians(config.latitudeE6/1000000.0),longitude=config.longitudeE6/1000000.0;
 double altitude[27];
 // Search the actual UTC+7 calendar interval. Normalizing an event's UTC
 // hour modulo 24 without restoring its date gives a whole-day error near
 // the international date line. Margins also find extrema at midnight.
 for(uint8_t i=0;i<27;i++)altitude[i]=SolarDetail::altitudeDifference(begin+(int(i)-1)*3600.0,latitude,longitude);
 for(uint8_t i=0;i<26;i++){
  double left=begin+(int(i)-1)*3600.0;
  SolarDetail::inspectInterval(left,left+3600,altitude[i],altitude[i+1],begin,end,latitude,longitude,result);
 }
 // An hourly sign scan can miss very short daylight/twilight near a polar
 // transition. Refine each sampled extremum and bracket its two sides.
 if(!result.sunriseValid||!result.sunsetValid)for(uint8_t i=1;i<26;i++){
  bool maximum=altitude[i]>altitude[i-1]&&altitude[i]>altitude[i+1];
  bool minimum=altitude[i]<altitude[i-1]&&altitude[i]<altitude[i+1];
  if(!maximum&&!minimum)continue;
  double left=begin+(int(i)-2)*3600.0,right=left+7200;
  double peak=SolarDetail::extremum(left,right,maximum,latitude,longitude);
  double value=SolarDetail::altitudeDifference(peak,latitude,longitude);
  SolarDetail::inspectInterval(left,peak,altitude[i-1],value,begin,end,latitude,longitude,result);
  SolarDetail::inspectInterval(peak,right,value,altitude[i+1],begin,end,latitude,longitude,result);
 }
 output=result;return true;
}

class SolarScheduler {
 SolarConfig known;
 bool configured=false,haveMinute=false;
 int64_t evaluatedMinute=0;
 bool fired[2]={false,false};
 int64_t lastFireDay[2]={0,0};
 bool cacheValid=false;
 int64_t cacheDay=0;
 int32_t cacheLatitude=0,cacheLongitude=0;
 SolarTimes cache[3],unavailable;
 void cacheFor(int64_t day,const SolarConfig& config){
  if(cacheValid&&cacheDay==day&&cacheLatitude==config.latitudeE6&&cacheLongitude==config.longitudeE6)return;
  cacheDay=day;cacheLatitude=config.latitudeE6;cacheLongitude=config.longitudeE6;cacheValid=true;
  for(uint8_t i=0;i<3;i++)calculateSolarTimes(day+int(i)-1,config,cache[i]);
 }
 public:
 bool begin(const SolarConfig& config){
  configured=false;haveMinute=false;cacheValid=false;
  for(uint8_t i=0;i<2;i++){fired[i]=false;lastFireDay[i]=0;}
  if(!validSolarConfig(config))return false;
  known=config;configured=true;return true;
 }
 void rebaseline(int64_t utc,bool clockReady){
  ScheduleWallTime wall;haveMinute=clockReady&&localScheduleTime(utc,wall);
  if(haveMinute)evaluatedMinute=wall.epochMinute;
 }
 bool configure(const SolarConfig& config,int64_t utc,bool clockReady){
  if(!validSolarConfig(config))return false;
  bool locationChanged=!configured||config.latitudeE6!=known.latitudeE6||config.longitudeE6!=known.longitudeE6;
  if(locationChanged||config.dawn!=known.dawn){fired[0]=false;lastFireDay[0]=0;}
  if(locationChanged||config.dusk!=known.dusk){fired[1]=false;lastFireDay[1]=0;}
  known=config;configured=true;rebaseline(utc,clockReady);return true;
 }
 const SolarTimes& today(int64_t utc,const SolarConfig& config){
  ScheduleWallTime wall;
  if(!validSolarConfig(config)||!localScheduleTime(utc,wall)){unavailable=SolarTimes();return unavailable;}
  cacheFor(wall.localDay,config);return cache[1];
 }
 template<class Emit> void tick(int64_t utc,bool clockReady,const SolarConfig& config,Emit emit){
  if(!validSolarConfig(config))return;
  if(!configured){if(!begin(config))return;}
  else if(config!=known){configure(config,utc,clockReady);return;}
  ScheduleWallTime wall;
  if(!clockReady||!localScheduleTime(utc,wall))return;
  if(!haveMinute){haveMinute=true;evaluatedMinute=wall.epochMinute;return;}
  int64_t delta=wall.epochMinute-evaluatedMinute;
  if(delta==0)return;
  evaluatedMinute=wall.epochMinute;
  if(delta<0||delta>1)return; // No late catch-up after clock steps or boot.
  if(!config.dawn.enabled&&!config.dusk.enabled)return;
  cacheFor(wall.localDay,config);
  for(uint8_t slot=0;slot<2;slot++){
   const SolarEntry& entry=slot==0?config.dawn:config.dusk;
   if(!entry.enabled)continue;
   // Offsets can move the source occurrence into yesterday or tomorrow.
   // Its configured weekday and RAM guard always belong to the SOURCE date.
   for(uint8_t i=0;i<3;i++){
    const SolarTimes& times=cache[i];
    if(slot==0?!times.sunriseValid:!times.sunsetValid)continue;
    uint8_t weekday=uint8_t((times.localDay+3)%7);
    if(!(entry.days&(1u<<weekday)))continue;
    int64_t eventUtc=(slot==0?times.sunriseUtc:times.sunsetUtc)+int64_t(entry.offset)*60;
    if(eventUtc/60!=wall.epochMinute||(fired[slot]&&times.localDay<=lastFireDay[slot]))continue;
    fired[slot]=true;lastFireDay[slot]=times.localDay;emit(slot==0,entry,eventUtc);
   }
  }
 }
};
} // namespace Deco
