#pragma once
#include <memory>
#include <new>
#include "Storage.h"
#include "Schedule.h"
#include "Solar.h"

namespace Deco {
inline void schedulesToJson(JsonObject object,const ScheduleConfig& config){
 object["enabled"]=config.enabled;
 JsonArray entries=object["entries"].to<JsonArray>();
 for(const auto& entry:config.entries){
  JsonObject value=entries.add<JsonObject>();
  value["enabled"]=entry.enabled;value["days"]=entry.days;value["minute"]=entry.minute;
  value["power"]=entry.power;value["favorite"]=entry.favorite;
 }
}
inline bool schedulesFromJson(JsonObjectConst object,ScheduleConfig& config){
 if(object.isNull()||object.size()!=2||!object["enabled"].is<bool>()||!object["entries"].is<JsonArrayConst>())return false;
 JsonArrayConst entries=object["entries"].as<JsonArrayConst>();
 if(entries.size()!=SCHEDULE_SLOTS)return false;
 ScheduleConfig next;next.enabled=object["enabled"].as<bool>();
 for(uint8_t i=0;i<SCHEDULE_SLOTS;i++){
  JsonObjectConst value=entries[i].as<JsonObjectConst>();int days,minute,favorite;
  if(value.isNull()||value.size()!=5||!value["enabled"].is<bool>()||!value["power"].is<bool>()||
     !readInt(value["days"],0,127,days)||!readInt(value["minute"],0,1439,minute)||!readInt(value["favorite"],-1,2,favorite))return false;
  auto& entry=next.entries[i];entry.enabled=value["enabled"].as<bool>();entry.power=value["power"].as<bool>();
  entry.days=days;entry.minute=minute;entry.favorite=favorite;
 }
 if(!validScheduleConfig(next))return false;
 config=next;return true;
}
inline void solarEntryToJson(JsonObject object,const SolarEntry& entry){
 object["enabled"]=entry.enabled;object["days"]=entry.days;object["offset"]=entry.offset;
 object["minutes"]=entry.minutes;object["favorite"]=entry.favorite;object["brightness"]=entry.brightness;
}
inline bool solarEntryFromJson(JsonObjectConst object,SolarEntry& entry){
 int days,offset,minutes,favorite,brightness;
 if(object.isNull()||object.size()!=6||!object["enabled"].is<bool>()||!readInt(object["days"],0,127,days)||
    !readInt(object["offset"],-120,120,offset)||!readInt(object["minutes"],1,120,minutes)||
    !readInt(object["favorite"],-1,2,favorite)||!readInt(object["brightness"],1,100,brightness))return false;
 SolarEntry next;next.enabled=object["enabled"].as<bool>();next.days=days;next.offset=offset;
 next.minutes=minutes;next.favorite=favorite;next.brightness=brightness;
 if(!validSolarEntry(next))return false;
 entry=next;return true;
}
inline void solarToJson(JsonObject object,const SolarConfig& config){
 object["latitudeE6"]=config.latitudeE6;object["longitudeE6"]=config.longitudeE6;
 solarEntryToJson(object["dawn"].to<JsonObject>(),config.dawn);
 solarEntryToJson(object["dusk"].to<JsonObject>(),config.dusk);
}
inline bool solarFromJson(JsonObjectConst object,SolarConfig& config){
 int latitude,longitude;
 if(object.isNull()||object.size()!=4||!readInt(object["latitudeE6"],-89000000,89000000,latitude)||
    !readInt(object["longitudeE6"],-180000000,180000000,longitude))return false;
 SolarConfig next;next.latitudeE6=latitude;next.longitudeE6=longitude;
 if(!solarEntryFromJson(object["dawn"].as<JsonObjectConst>(),next.dawn)||
    !solarEntryFromJson(object["dusk"].as<JsonObjectConst>(),next.dusk)||!validSolarConfig(next))return false;
 config=next;return true;
}
inline String encodeSchedules(const ScheduleConfig& schedules,const SolarConfig& sun){
 JsonDocument document;
 schedulesToJson(document["schedules"].to<JsonObject>(),schedules);
 solarToJson(document["sun"].to<JsonObject>(),sun);
 String text;serializeJson(document,text);return text;
}
inline bool decodeSchedules(const String& text,ScheduleConfig& schedules,SolarConfig& sun){
 JsonDocument document;
 if(deserializeJson(document,text)||!document.is<JsonObject>()||document.size()!=2)return false;
 ScheduleConfig nextSchedules;SolarConfig nextSun;
 if(!schedulesFromJson(document["schedules"].as<JsonObjectConst>(),nextSchedules)||
    !solarFromJson(document["sun"].as<JsonObjectConst>(),nextSun))return false;
 schedules=nextSchedules;sun=nextSun;return true;
}

class ScheduleStorage {
 static constexpr uint32_t MAGIC=0x4e414c50u; // PLAN, a separate NVS record type.
 static constexpr uint8_t SCHEMA=1;
 static constexpr size_t HEADER_BYTES=16,MAX_BLOB_BYTES=4096;
 Preferences prefs;bool opened=false;int slot=-1;uint32_t generation=0;
 String saved;
 enum class Record {Missing,Invalid,Valid,Future,NoMemory};
 Record read(const char* key,String& payload,uint32_t& gen){
  size_t size=prefs.getBytesLength(key);
  if(!size)return Record::Missing;
  // An oversized record cannot be safely interpreted by this firmware;
  // retain it rather than replacing potentially newer configuration.
  if(size>MAX_BLOB_BYTES)return Record::Future;
  if(size<HEADER_BYTES)return Record::Invalid;
  std::unique_ptr<uint8_t[]> bytes(new(std::nothrow) uint8_t[size]);
  if(!bytes)return Record::NoMemory;
  if(prefs.getBytes(key,bytes.get(),size)!=size||get32(bytes.get())!=MAGIC||
     get32(bytes.get()+8)!=size-HEADER_BYTES||get32(bytes.get()+12)!=crc32(bytes.get()+HEADER_BYTES,size-HEADER_BYTES))return Record::Invalid;
  uint32_t meta=get32(bytes.get()+4);uint8_t schema=uint8_t(meta);
  if(schema>SCHEMA)return Record::Future;
  if(schema!=SCHEMA)return Record::Invalid;
  gen=meta>>8;payload=String(reinterpret_cast<const char*>(bytes.get()+HEADER_BYTES),size-HEADER_BYTES);
  return payload.length()==size-HEADER_BYTES?Record::Valid:Record::NoMemory;
 }
 bool write(const char* key,const String& payload,uint32_t gen){
  if(payload.length()>MAX_BLOB_BYTES-HEADER_BYTES)return false;
  size_t size=payload.length()+HEADER_BYTES;
  std::unique_ptr<uint8_t[]> bytes(new(std::nothrow) uint8_t[size]);
  if(!bytes)return false;
  put32(bytes.get(),MAGIC);put32(bytes.get()+4,((gen&0xffffffu)<<8)|SCHEMA);
  put32(bytes.get()+8,payload.length());put32(bytes.get()+12,crc32(reinterpret_cast<const uint8_t*>(payload.c_str()),payload.length()));
  memcpy(bytes.get()+HEADER_BYTES,payload.c_str(),payload.length());
  if(prefs.putBytes(key,bytes.get(),size)!=size)return false;
  // Release the outgoing allocation before allocating the readback buffer.
  bytes.reset();String check;uint32_t checkGeneration=0;
  return read(key,check,checkGeneration)==Record::Valid&&check==payload&&checkGeneration==(gen&0xffffffu);
 }
 public:
 bool blocked=false,hasSaved=false;String error;uint32_t writes=0;
 bool begin(ScheduleConfig& schedules,SolarConfig& sun){
  blocked=false;hasSaved=false;writes=0;slot=-1;generation=0;saved=String();error=String();
  opened=prefs.begin("deco-plan",false);
  if(!opened){error="Không mở được NVS lịch";return false;}
  String a,b;uint32_t ga=0,gb=0;ScheduleConfig sa,sb;SolarConfig ua,ub;
  Record ra=read("slot-a",a,ga),rb=read("slot-b",b,gb);
  if(ra==Record::Future||rb==Record::Future){blocked=true;error="Dữ liệu lịch mới hơn firmware; không ghi đè NVS";return false;}
  if(ra==Record::NoMemory||rb==Record::NoMemory){error="Thiếu bộ nhớ đọc lịch";return false;}
  bool va=ra==Record::Valid&&decodeSchedules(a,sa,ua),vb=rb==Record::Valid&&decodeSchedules(b,sb,ub);
  hasSaved=va||vb;
  // Compare 24-bit generations modulo their range, including wrap to zero.
  if(va&&(!vb||(ga!=gb&&((ga-gb)&0xffffffu)<0x800000u))){schedules=sa;sun=ua;slot=0;generation=ga;saved=a;}
  else if(vb){schedules=sb;sun=ub;slot=1;generation=gb;saved=b;}
  else if(ra!=Record::Missing||rb!=Record::Missing){error="Bản lưu lịch lỗi; đang dùng mặc định";return false;}
  return true;
 }
 bool save(const ScheduleConfig& schedules,const SolarConfig& sun){
  if(!opened||blocked){error=blocked?"Không thể ghi dữ liệu lịch chưa hỗ trợ":"NVS lịch chưa sẵn sàng";return false;}
  if(!validScheduleConfig(schedules)||!validSolarConfig(sun)){error="Cấu hình lịch không hợp lệ";return false;}
  String text=encodeSchedules(schedules,sun);
  // Verify the serialized representation before touching the previous slot.
  ScheduleConfig checkedSchedules;SolarConfig checkedSun;
  if(!decodeSchedules(text,checkedSchedules,checkedSun)||checkedSchedules!=schedules||!(checkedSun==sun)){
   error="Không mã hóa được lịch";return false;
  }
  if(hasSaved&&text==saved){error=String();return true;}
  int next=slot==0?1:0;uint32_t nextGeneration=(generation+1)&0xffffffu;
  if(!write(next==0?"slot-a":"slot-b",text,nextGeneration)){error="Ghi hoặc kiểm tra NVS lịch thất bại";return false;}
  slot=next;generation=nextGeneration;saved=text;hasSaved=true;++writes;error=String();return true;
 }
};
}
