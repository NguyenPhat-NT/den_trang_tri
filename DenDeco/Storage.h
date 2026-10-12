#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "Core.h"
#include "NetworkPolicy.h"
namespace Deco {
// Schema 2 extends the supported mode IDs. Schema 1 remains readable; older
// firmware detects schema 2 as future data instead of overwriting new presets.
constexpr uint8_t LIGHT_SCHEMA_VERSION=2;
inline uint32_t crc32(const uint8_t* p,size_t n){uint32_t c=0xffffffffu;while(n--){c^=*p++;for(int k=0;k<8;k++)c=(c>>1)^(0xedb88320u&(0u-(c&1)));}return ~c;}
inline void put32(uint8_t* p,uint32_t n){for(int i=0;i<4;i++)p[i]=uint8_t(n>>(8*i));}
inline uint32_t get32(const uint8_t* p){uint32_t n=0;for(int i=0;i<4;i++)n|=uint32_t(p[i])<<(8*i);return n;}
inline void lightToJson(JsonObject o,const Light& l){o["mode"]=l.mode;o["r"]=l.r;o["g"]=l.g;o["b"]=l.b;o["brightness"]=l.brightness;o["speed"]=l.speed;}
inline bool readInt(JsonVariantConst v,int lo,int hi,int& n){if(!v.is<int>())return false;n=v.as<int>();return n>=lo&&n<=hi;}
inline bool lightFromJson(JsonObjectConst o,Light& l){
 int mode,r,g,b,p,s;if(!readInt(o["mode"],0,ModeCount-1,mode)||!readInt(o["r"],0,255,r)||!readInt(o["g"],0,255,g)||!readInt(o["b"],0,255,b)||!readInt(o["brightness"],1,100,p)||!readInt(o["speed"],1,100,s))return false;
 l.mode=mode;l.r=r;l.g=g;l.b=b;l.brightness=p;l.speed=s;return true;
}
inline String encodeSettings(const Settings& s){JsonDocument d;d["power"]=s.power;lightToJson(d["light"].to<JsonObject>(),s.light);d["sleepBrightness"]=s.sleepBrightness;d["cap"]=s.cap;JsonArray a=d["favorites"].to<JsonArray>();for(auto& f:s.favorites){JsonObject o=a.add<JsonObject>();o["valid"]=f.valid;lightToJson(o["light"].to<JsonObject>(),f.light);}String result;serializeJson(d,result);return result;}
inline bool decodeSettings(const String& text,Settings& s){
 JsonDocument d;if(deserializeJson(d,text)||!d["power"].is<bool>())return false;Settings n;n.power=d["power"].as<bool>();
 int sleep,cap;if(!lightFromJson(d["light"].as<JsonObjectConst>(),n.light)||!readInt(d["sleepBrightness"],1,100,sleep)||!readInt(d["cap"],Config::MIN_CAP_PERCENT,Config::MAX_CAP_PERCENT,cap))return false;
 n.sleepBrightness=sleep;n.cap=cap;JsonArrayConst a=d["favorites"].as<JsonArrayConst>();if(a.size()!=3)return false;
 for(int i=0;i<3;i++){if(!a[i]["valid"].is<bool>()||!lightFromJson(a[i]["light"].as<JsonObjectConst>(),n.favorites[i].light))return false;n.favorites[i].valid=a[i]["valid"].as<bool>();}
 if(!validSettings(n))return false;
 s=n;return true;
}
class Storage {
 Preferences prefs;bool opened=false;int slot=-1;uint32_t generation=0,lastChange=0,lastAttempt=0;bool tried=false;
 String saved;
 bool readBlob(const char* key,String& payload,uint32_t& gen,bool& future){
  size_t n=prefs.getBytesLength(key);if(n<16||n>2048)return false;uint8_t bytes[2048];if(prefs.getBytes(key,bytes,n)!=n)return false;
  if(get32(bytes)!=0x4f434544u||get32(bytes+8)!=n-16||get32(bytes+12)!=crc32(bytes+16,n-16))return false;
  uint32_t meta=get32(bytes+4);uint8_t schema=uint8_t(meta);if(schema>LIGHT_SCHEMA_VERSION){future=true;return false;}if(schema<1)return false;
  gen=meta>>8;payload=String(reinterpret_cast<const char*>(bytes+16),n-16);return true;
 }
 bool writeBlob(const char* key,const String& payload,uint32_t gen){
  if(payload.length()>2032)return false;
  uint8_t bytes[2048];put32(bytes,0x4f434544u);put32(bytes+4,((gen&0xffffffu)<<8)|LIGHT_SCHEMA_VERSION);put32(bytes+8,payload.length());put32(bytes+12,crc32(reinterpret_cast<const uint8_t*>(payload.c_str()),payload.length()));memcpy(bytes+16,payload.c_str(),payload.length());
  if(prefs.putBytes(key,bytes,payload.length()+16)!=payload.length()+16)return false;
  String check;uint32_t readGen=0;bool future=false;return readBlob(key,check,readGen,future)&&check==payload&&readGen==(gen&0xffffffu);
 }
 public:
 bool dirty=false,blocked=false,hasSaved=false;String error;uint32_t writes=0;
 void begin(Settings& s){
  opened=prefs.begin("deco-light",false);if(!opened){error="Không mở được NVS";saved=encodeSettings(s);return;}
  String a,b;uint32_t ga=0,gb=0;bool future=false;Settings sa,sb;bool va=readBlob("slot-a",a,ga,future)&&decodeSettings(a,sa),vb=readBlob("slot-b",b,gb,future)&&decodeSettings(b,sb);
  blocked=future;if(blocked){error="Schema mới hơn firmware; không ghi đè NVS";saved=encodeSettings(s);return;}
  // 24-bit modular generation comparison.
  hasSaved=va||vb;
  if(va&&(!vb||(((ga-gb)&0xffffffu)<0x800000u&&ga!=gb))){s=sa;slot=0;generation=ga;saved=a;}
  else if(vb){s=sb;slot=1;generation=gb;saved=b;}
  else {saved=encodeSettings(s);if(prefs.getBytesLength("slot-a")||prefs.getBytesLength("slot-b"))error="Bản lưu lỗi; đang dùng mặc định";}
 }
 void changed(const Settings& s,uint32_t now){dirty=encodeSettings(s)!=saved;if(dirty){lastChange=now;tried=false;} }
 bool saveNow(const Settings& s){
  if(!dirty)return !blocked;
  if(!opened||blocked){error=blocked?"Không thể ghi schema chưa hỗ trợ":"NVS chưa sẵn sàng";return false;}
  String text=encodeSettings(s);int next=slot==0?1:0;uint32_t gen=(generation+1)&0xffffffu;
  if(!writeBlob(next==0?"slot-a":"slot-b",text,gen)){error="Ghi hoặc kiểm tra NVS thất bại";return false;}
  saved=text;generation=gen;slot=next;dirty=false;hasSaved=true;error="";writes++;return true;
 }
 void tick(const Settings& s,uint32_t now){if(!dirty||uint32_t(now-lastChange)<Config::SAVE_IDLE_MS||(tried&&uint32_t(now-lastAttempt)<Config::SAVE_RETRY_MS))return;lastAttempt=now;tried=true;saveNow(s);}
};
struct NetworkConfig {String name="Đèn Deco",ssid,password,hostname="den-deco";};
inline bool validNetworkConfig(const NetworkConfig& c){
 return c.name.length()>=1&&c.name.length()<=48&&c.ssid.length()<=32&&c.password.length()<=64&&validHostname(c.hostname.c_str());
}
class NetworkStorage {
 Preferences prefs;bool opened=false;
 public:
 bool blocked=false;
 bool begin(NetworkConfig& c){
  blocked=false;
  opened=prefs.begin("deco-network",false);
  if(!opened)return false;
  size_t n=prefs.getBytesLength("config");
  if(!n)return true;
  if(n<4||n>768)return false;
  uint8_t bytes[768];
  if(prefs.getBytes("config",bytes,n)!=n||get32(bytes)!=crc32(bytes+4,n-4))return false;
  JsonDocument d;
  if(deserializeJson(d,bytes+4,n-4)||!d["schema"].is<uint32_t>())return false;
  uint32_t schema=d["schema"].as<uint32_t>();
  // An authentic future version is kept intact. A damaged CRC, however,
  // leaves provisioning available so a supported configuration can repair it.
  if(schema>2){blocked=true;return false;}
  if(schema<1||!d["name"].is<String>()||!d["ssid"].is<String>()||!d["password"].is<String>())return false;
  NetworkConfig v;v.name=d["name"].as<String>();v.ssid=d["ssid"].as<String>();v.password=d["password"].as<String>();
  if(schema==1){
   if(!d["apPassword"].is<String>())return false;
   String oldAP=d["apPassword"].as<String>();
   if(oldAP.length()!=0&&oldAP.length()!=12)return false;
   // Preserve the existing home WiFi credentials while retiring the AP key.
   v.hostname="den-deco";
  }else{
   if(!d["hostname"].is<String>())return false;
   v.hostname=d["hostname"].as<String>();
  }
  if(!validNetworkConfig(v))return false;
  c=v;return true;
 }
 bool save(const NetworkConfig& c){
  if(!opened||blocked||!validNetworkConfig(c))return false;
  JsonDocument d;d["schema"]=2;d["name"]=c.name;d["ssid"]=c.ssid;d["password"]=c.password;d["hostname"]=c.hostname;
  String text;serializeJson(d,text);
  if(text.length()>764)return false;
  uint8_t bytes[768];put32(bytes,crc32(reinterpret_cast<const uint8_t*>(text.c_str()),text.length()));memcpy(bytes+4,text.c_str(),text.length());size_t n=text.length()+4;
  if(prefs.putBytes("config",bytes,n)!=n)return false;
  uint8_t check[768];return prefs.getBytesLength("config")==n&&prefs.getBytes("config",check,n)==n&&memcmp(bytes,check,n)==0;
 }
};
}
