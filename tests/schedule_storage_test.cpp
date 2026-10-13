#include "../DenDeco/ScheduleStorage.h"
#include <cassert>
#include <iostream>
#include <vector>
FakeNVS fakeNVS;
using namespace Deco;

static ScheduleConfig weekly(){
 ScheduleConfig config;config.enabled=true;
 for(uint8_t i=0;i<SCHEDULE_SLOTS;i++){
  auto& entry=config.entries[i];entry.enabled=true;entry.days=uint8_t(1u<<(i%7));
  entry.minute=i*180;entry.power=i%2==0;entry.favorite=entry.power?int8_t(i%3):-1;
 }
 return config;
}
static SolarConfig solar(){
 SolarConfig config;config.latitudeE6=-89000000;config.longitudeE6=180000000;
 config.dawn.enabled=true;config.dawn.days=127;config.dawn.offset=-120;config.dawn.minutes=1;config.dawn.favorite=-1;config.dawn.brightness=1;
 config.dusk.enabled=true;config.dusk.days=1;config.dusk.offset=120;config.dusk.minutes=120;config.dusk.favorite=2;config.dusk.brightness=100;
 return config;
}
static JsonDocument document(){
 JsonDocument value;schedulesToJson(value["schedules"].to<JsonObject>(),weekly());solarToJson(value["sun"].to<JsonObject>(),solar());return value;
}
static void invalid(JsonDocument& value){
 ScheduleConfig expected=weekly(),read=expected;SolarConfig expectedSun=solar(),readSun=expectedSun;
 String text;serializeJson(value,text);bool accepted=decodeSchedules(text,read,readSun);
 if(accepted)std::cerr<<"Unexpectedly accepted malformed fixture: "<<text.c_str()<<"\n";
 assert(!accepted);
 assert(read==expected&&readSun==expectedSun);
}
static void testJson(){
 ScheduleConfig expected=weekly(),read;SolarConfig expectedSun=solar(),readSun;
 String text=encodeSchedules(expected,expectedSun);assert(text.length()<1800);
 assert(decodeSchedules(text,read,readSun)&&read==expected&&readSun==expectedSun);
 JsonDocument value=document();assert(value["schedules"]["enabled"].is<bool>());
 assert(value["schedules"]["entries"].size()==8&&value["sun"]["latitudeE6"].is<int32_t>());
 assert(value["sun"]["dawn"]["offset"].as<int>()==-120);
 ScheduleConfig defaults;SolarConfig defaultSun;
 assert(decodeSchedules(encodeSchedules(defaults,defaultSun),read,readSun)&&read==defaults&&readSun==defaultSun);
 // Both coordinate endpoints and all integer bounds survive the wire format.
 expectedSun.latitudeE6=89000000;expectedSun.longitudeE6=-180000000;
 expected.entries[7].minute=1439;expected.entries[7].days=127;
 assert(decodeSchedules(encodeSchedules(expected,expectedSun),read,readSun)&&read==expected&&readSun==expectedSun);

 value=document();value["schedules"]["enabled"]=1;invalid(value);
 value=document();value["schedules"]["enabled"]="true";invalid(value);
 value=document();value["schedules"].remove("enabled");invalid(value);
 value=document();value["schedules"]["other"]=false;invalid(value);
 value=document();value["schedules"]["entries"].as<JsonArray>().remove(7);invalid(value);
 value=document();value["schedules"]["entries"].as<JsonArray>().add<JsonObject>();invalid(value);
 value=document();value["schedules"]["entries"]=JsonObject();invalid(value);
 value=document();value["schedules"]["entries"][0]=0;invalid(value);
 value=document();value["schedules"]["entries"][0].remove("power");invalid(value);
 value=document();value["schedules"]["entries"][0]["enabled"]="false";invalid(value);
 value=document();value["schedules"]["entries"][0]["power"]=1;invalid(value);
 for(int bad:{-1,128}){value=document();value["schedules"]["entries"][0]["days"]=bad;invalid(value);}
 for(int bad:{-1,1440}){value=document();value["schedules"]["entries"][0]["minute"]=bad;invalid(value);}
 for(int bad:{-2,3}){value=document();value["schedules"]["entries"][0]["favorite"]=bad;invalid(value);}
 value=document();value["schedules"]["entries"][0]["days"]=true;invalid(value);
 value=document();value["schedules"]["entries"][0]["minute"]=30.5;invalid(value);
 value=document();value["schedules"]["entries"][0]["favorite"]="1";invalid(value);
 value=document();value["schedules"]["entries"][0]["days"]=0;invalid(value);
 value=document();value["schedules"]["entries"][0]["favorite"]=-1;invalid(value); // ON needs a favorite.
 value=document();value["schedules"]["entries"][1]["favorite"]=0;invalid(value); // OFF has no favorite.
 value=document();value["schedules"]["entries"][0]["unexpected"]=0;invalid(value);

 value=document();value["sun"].remove("dawn");invalid(value);
 value=document();value["sun"]["longitudeE6"]="180000000";invalid(value);
 value=document();value["sun"]["latitudeE6"]=10.776;invalid(value);
 for(int bad:{-89000001,89000001}){value=document();value["sun"]["latitudeE6"]=bad;invalid(value);}
 for(int bad:{-180000001,180000001}){value=document();value["sun"]["longitudeE6"]=bad;invalid(value);}
 value=document();value["sun"]["dawn"]["enabled"]=0;invalid(value);
 value=document();value["sun"]["dawn"]["days"]=0;invalid(value);
 for(int bad:{-1,128}){value=document();value["sun"]["dawn"]["days"]=bad;invalid(value);}
 for(int bad:{-121,121}){value=document();value["sun"]["dawn"]["offset"]=bad;invalid(value);}
 for(int bad:{0,121}){value=document();value["sun"]["dawn"]["minutes"]=bad;invalid(value);}
 for(int bad:{-2,3}){value=document();value["sun"]["dawn"]["favorite"]=bad;invalid(value);}
 for(int bad:{0,101}){value=document();value["sun"]["dawn"]["brightness"]=bad;invalid(value);}
 value=document();value["sun"]["dawn"].remove("minutes");invalid(value);
 value=document();value["sun"]["dusk"]["offset"]=false;invalid(value);
 value=document();value["sun"]["dusk"]["minutes"]=20.5;invalid(value);
 value=document();value["sun"]["dusk"]["brightness"]="35";invalid(value);
 value=document();value["sun"]["dusk"]["enabled"]=false;value["sun"]["dusk"]["offset"]=121;invalid(value); // Disabled rows still validate.
 value=document();value["sun"]["dusk"]["unexpected"]=0;invalid(value);
 value=document();value["unexpected"]=0;invalid(value);
 assert(!decodeSchedules(String("{}"),read,readSun)&&!decodeSchedules(String("[]"),read,readSun));
 assert(!decodeSchedules(String("{bad"),read,readSun));
}

static void record(const char* key,const String& text,uint8_t schema,uint32_t generation){
 auto& bytes=fakeNVS.data[std::string("deco-plan/")+key];bytes.resize(text.length()+16);
 put32(bytes.data(),0x4e414c50u);put32(bytes.data()+4,((generation&0xffffffu)<<8)|schema);
 put32(bytes.data()+8,text.length());put32(bytes.data()+12,crc32(reinterpret_cast<const uint8_t*>(text.c_str()),text.length()));
 memcpy(bytes.data()+16,text.c_str(),text.length());
}
static void testStorage(){
 fakeNVS=FakeNVS{};ScheduleConfig defaults;SolarConfig defaultSun;ScheduleStorage fresh;
 assert(fresh.begin(defaults,defaultSun)&&!fresh.hasSaved&&!fresh.blocked&&fresh.error.length()==0);
 const auto initial=fakeNVS.data;assert(!fakeNVS.data["deco-plan/slot-a"].size());
 ScheduleConfig first=weekly();SolarConfig sun=solar();assert(fresh.save(first,sun)&&fresh.hasSaved&&fresh.writes==1);
 assert(get32(fakeNVS.data["deco-plan/slot-a"].data()+4)==(1u<<8|1u));
 const auto firstBytes=fakeNVS.data;
 assert(fresh.save(first,sun)&&fakeNVS.data==firstBytes&&fresh.writes==1); // No flash write for unchanged data.
 ScheduleConfig second=first;second.entries[0].minute=59;SolarConfig secondSun=sun;secondSun.latitudeE6=10776000;
 assert(fresh.save(second,secondSun)&&fresh.writes==2);assert(get32(fakeNVS.data["deco-plan/slot-b"].data()+4)==(2u<<8|1u));
 ScheduleConfig loaded;SolarConfig loadedSun;ScheduleStorage restore;
 assert(restore.begin(loaded,loadedSun)&&restore.hasSaved&&!restore.blocked&&loaded==second&&loadedSun==secondSun);
 const auto goodData=fakeNVS.data;
 ScheduleConfig invalidWeekly=second;invalidWeekly.entries[0].minute=1440;
 assert(!restore.save(invalidWeekly,secondSun)&&fakeNVS.data==goodData);
 SolarConfig invalidSun=secondSun;invalidSun.dawn.minutes=0;
 assert(!restore.save(second,invalidSun)&&fakeNVS.data==goodData);
 ScheduleConfig third=second;third.entries[0].minute=60;
 fakeNVS.failWrite=true;assert(!restore.save(third,secondSun)&&fakeNVS.data==goodData&&restore.error.length()>0&&restore.writes==0);fakeNVS.failWrite=false;
 fakeNVS.corruptWrite=true;assert(!restore.save(third,secondSun)&&restore.error.length()>0&&restore.writes==0);fakeNVS.corruptWrite=false;
 // A write that reports success but fails CRC readback never replaces the
 // known-good slot. Reboot recovers the most recent verified record.
 ScheduleConfig recovered;SolarConfig recoveredSun;ScheduleStorage afterCorrupt;
 assert(afterCorrupt.begin(recovered,recoveredSun)&&recovered==second&&recoveredSun==secondSun);
 fakeNVS.torn=true;assert(!restore.save(third,secondSun));fakeNVS.torn=false;
 ScheduleStorage afterTorn;assert(afterTorn.begin(recovered,recoveredSun)&&recovered==second&&recoveredSun==secondSun);
 assert(restore.save(third,secondSun)&&restore.error.length()==0);
 ScheduleStorage retried;assert(retried.begin(recovered,recoveredSun)&&recovered==third&&recoveredSun==secondSun);
 // Damaged headers, CRC payloads and unsupported JSON fall back atomically.
 for(int damage:{0,8,12,16}){
  fakeNVS=FakeNVS{};fakeNVS.data=goodData;fakeNVS.data["deco-plan/slot-b"][damage]^=1;
  ScheduleStorage fallback;assert(fallback.begin(recovered,recoveredSun)&&recovered==first&&recoveredSun==sun);
 }
 fakeNVS=FakeNVS{};fakeNVS.data=goodData;record("slot-b",String("{}"),1,3);
 ScheduleStorage badJson;assert(badJson.begin(recovered,recoveredSun)&&recovered==first&&recoveredSun==sun);
 fakeNVS=FakeNVS{};fakeNVS.data["deco-plan/slot-a"]={1,2,3};
 ScheduleConfig untouched=second;SolarConfig untouchedSun=secondSun;ScheduleStorage invalidRecord;
 assert(!invalidRecord.begin(untouched,untouchedSun)&&!invalidRecord.blocked&&!invalidRecord.hasSaved&&untouched==second&&untouchedSun==secondSun);
 assert(invalidRecord.save(first,sun));ScheduleStorage repaired;
 assert(repaired.begin(recovered,recoveredSun)&&recovered==first&&recoveredSun==sun);
 // The separate namespace leaves the earlier light and network records alone.
 fakeNVS.data["deco-light/slot-a"]={11,22};fakeNVS.data["deco-network/config"]={33,44};
 assert(repaired.save(second,secondSun));assert(fakeNVS.data["deco-light/slot-a"]==std::vector<uint8_t>({11,22}));
 assert(fakeNVS.data["deco-network/config"]==std::vector<uint8_t>({33,44}));
 fakeNVS=FakeNVS{};fakeNVS.failOpen=true;ScheduleStorage unavailable;
 assert(!unavailable.begin(defaults,defaultSun)&&!unavailable.save(first,sun)&&unavailable.error.length()>0);
 (void)initial;
}
static void testGenerationsAndFuture(){
 ScheduleConfig a=weekly(),b=a;b.entries[0].minute=59;SolarConfig sun=solar();
 String ta=encodeSchedules(a,sun),tb=encodeSchedules(b,sun);ScheduleConfig loaded;SolarConfig loadedSun;
 fakeNVS=FakeNVS{};record("slot-a",ta,1,0xffffffu);record("slot-b",tb,1,0);
 ScheduleStorage wrapped;assert(wrapped.begin(loaded,loadedSun)&&loaded==b);
 assert(wrapped.save(a,sun));assert(get32(fakeNVS.data["deco-plan/slot-a"].data()+4)==(1u<<8|1u));
 fakeNVS=FakeNVS{};record("slot-a",ta,1,0);record("slot-b",tb,1,0xffffffu);
 ScheduleStorage reversed;assert(reversed.begin(loaded,loadedSun)&&loaded==a);
 fakeNVS=FakeNVS{};record("slot-a",ta,1,7);record("slot-b",tb,1,7);
 ScheduleStorage tied;assert(tied.begin(loaded,loadedSun)&&loaded==b);
 fakeNVS=FakeNVS{};record("slot-a",ta,1,6);record("slot-b",String("{\"unknownFuture\":true}"),2,7);
 const auto futureData=fakeNVS.data;ScheduleStorage future;ScheduleConfig defaults;SolarConfig defaultSun;
 assert(!future.begin(defaults,defaultSun)&&future.blocked&&!future.hasSaved&&defaults==ScheduleConfig{}&&defaultSun==SolarConfig{});
 assert(!future.save(a,sun)&&fakeNVS.data==futureData);
 fakeNVS.data["deco-plan/slot-b"].back()^=1;ScheduleStorage corruptFuture;
 assert(corruptFuture.begin(loaded,loadedSun)&&!corruptFuture.blocked&&loaded==a&&corruptFuture.save(b,sun));
 fakeNVS=FakeNVS{};record("slot-a",ta,0,1);ScheduleStorage zeroSchema;
 assert(!zeroSchema.begin(defaults,defaultSun)&&!zeroSchema.blocked&&zeroSchema.save(a,sun));
 fakeNVS=FakeNVS{};record("slot-a",ta,255,1);ScheduleStorage distant;
 assert(!distant.begin(defaults,defaultSun)&&distant.blocked&&!distant.save(a,sun));
 // Oversized unrecognized records are retained, not overwritten as defaults.
 fakeNVS=FakeNVS{};fakeNVS.data["deco-plan/slot-a"]=std::vector<uint8_t>(4097,0);const auto oversized=fakeNVS.data;
 ScheduleStorage tooLarge;assert(!tooLarge.begin(defaults,defaultSun)&&tooLarge.blocked&&!tooLarge.save(a,sun)&&fakeNVS.data==oversized);
}
int main(){
 testJson();testStorage();testGenerationsAndFuture();
 std::cout<<"PASS: strict weekly/solar JSON and atomic decoding; independent dual-slot schedule NVS, CRC/header recovery, immediate/no-op save, readback/torn/write/open failures, generation wrap and future protection\n";
}
