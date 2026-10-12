#include "../DenDeco/Storage.h"
#include <cassert>
#include <iostream>
FakeNVS fakeNVS;
using namespace Deco;
static void putLightRecord(const char* key,const Settings& settings,uint8_t schema,uint32_t generation){
 String text=encodeSettings(settings);auto& bytes=fakeNVS.data[std::string("deco-light/")+key];bytes.resize(text.length()+16);
 put32(bytes.data(),0x4f434544u);put32(bytes.data()+4,((generation&0xffffffu)<<8)|schema);
 put32(bytes.data()+8,text.length());put32(bytes.data()+12,crc32(reinterpret_cast<const uint8_t*>(text.c_str()),text.length()));
 memcpy(bytes.data()+16,text.c_str(),text.length());
}
static void testLightUpgrade(){
 // Records written by 1.1.0 must retain every field and every original mode ID.
 for(uint8_t mode=0;mode<6;mode++){
  fakeNVS=FakeNVS{};Settings legacy;legacy.power=true;legacy.light.mode=mode;
  legacy.light.r=17;legacy.light.g=43;legacy.light.b=92;legacy.light.brightness=37;legacy.light.speed=68;
  legacy.sleepBrightness=11;legacy.cap=22;
  for(int i=0;i<3;i++){legacy.favorites[i].valid=true;legacy.favorites[i].light=legacy.light;legacy.favorites[i].light.mode=i*2;}
  putLightRecord("slot-a",legacy,1,17);
  Settings loaded;Storage upgrade;upgrade.begin(loaded);
  assert(loaded==legacy&&upgrade.hasSaved&&!upgrade.dirty&&!upgrade.blocked&&upgrade.writes==0);
  // New modes and favorites are saved with schema2, while the legacy slot
  // stays intact. A torn write can still fall back to that legacy record.
  loaded.light.mode=13;loaded.favorites[1].light.mode=12;loaded.favorites[2].light.mode=8;
  upgrade.changed(loaded,100);assert(upgrade.saveNow(loaded));
  assert(uint8_t(get32(fakeNVS.data["deco-light/slot-b"].data()+4))==2);
  assert(uint8_t(get32(fakeNVS.data["deco-light/slot-a"].data()+4))==1);
  Settings restored;Storage newReader;newReader.begin(restored);assert(restored==loaded&&!newReader.blocked);
  auto& latest=fakeNVS.data["deco-light/slot-b"];latest.resize(latest.size()/2);
  Settings recovered;Storage recovery;recovery.begin(recovered);assert(recovered==legacy);
 }
 // Mixed schema generations select the newest slot even across generation wrap.
 fakeNVS=FakeNVS{};Settings old,newer;old.light.mode=5;newer.light.mode=9;
 putLightRecord("slot-a",old,1,0xffffffu);putLightRecord("slot-b",newer,2,0);
 Settings selected;Storage mixed;mixed.begin(selected);assert(selected==newer);
 const auto original=fakeNVS.data["deco-light/slot-b"];
 // A future schema protects both records from being overwritten by this build.
 putLightRecord("slot-a",newer,3,1);Settings future;Storage protectedReader;protectedReader.begin(future);
 assert(protectedReader.blocked);future.power=true;protectedReader.changed(future,100);
 assert(!protectedReader.saveNow(future)&&fakeNVS.data["deco-light/slot-b"]==original);
 fakeNVS=FakeNVS{};
}
static void putNetworkJson(const String& text){
 auto& bytes=fakeNVS.data["deco-network/config"];bytes.resize(text.length()+4);
 put32(bytes.data(),crc32(reinterpret_cast<const uint8_t*>(text.c_str()),text.length()));
 memcpy(bytes.data()+4,text.c_str(),text.length());
}
static void testNetworkStorage(){
 fakeNVS=FakeNVS{};
 NetworkStorage network;NetworkConfig n;
 assert(network.begin(n)&&!network.blocked&&n.hostname==String("den-deco"));
 n.name="Đèn phòng ngủ";n.ssid="Home";n.password="testing123";n.hostname="den-phong-ngu";
 assert(network.save(n));
 NetworkStorage network2;NetworkConfig read;
 assert(network2.begin(read)&&read.name==n.name&&read.password==n.password&&read.ssid==n.ssid&&read.hostname==n.hostname);
 const auto& newBytes=fakeNVS.data["deco-network/config"];JsonDocument saved;
 assert(!deserializeJson(saved,newBytes.data()+4,newBytes.size()-4));
 assert(saved["schema"].as<int>()==2&&saved["hostname"].as<String>()==n.hostname&&saved["apPassword"].isNull());

 // Read a hand-built version 1 record, retaining home WiFi and display name.
 putNetworkJson(String("{\"schema\":1,\"name\":\"Den cu\",\"ssid\":\"OldHome\",\"password\":\"old-secret\",\"apPassword\":\"ABCDabcd1234\"}"));
 NetworkStorage legacy;NetworkConfig migrated;
 assert(legacy.begin(migrated)&&!legacy.blocked);
 assert(migrated.name==String("Den cu")&&migrated.ssid==String("OldHome")&&migrated.password==String("old-secret")&&migrated.hostname==String("den-deco"));
 assert(legacy.save(migrated));NetworkStorage upgraded;NetworkConfig upgradeRead;
 assert(upgraded.begin(upgradeRead)&&upgradeRead.ssid==migrated.ssid&&upgradeRead.password==migrated.password&&upgradeRead.name==migrated.name);
 auto& upgradedBytes=fakeNVS.data["deco-network/config"];JsonDocument upgradeJson;
 assert(!deserializeJson(upgradeJson,upgradedBytes.data()+4,upgradedBytes.size()-4));
 assert(upgradeJson["schema"].as<int>()==2&&upgradeJson["apPassword"].isNull());
 // Empty legacy AP keys were valid, too; malformed old keys are rejected.
 putNetworkJson(String("{\"schema\":1,\"name\":\"Den cu\",\"ssid\":\"OldHome\",\"password\":\"old-secret\",\"apPassword\":\"\"}"));
 NetworkStorage oldEmpty;NetworkConfig oldEmptyConfig;assert(oldEmpty.begin(oldEmptyConfig));
 putNetworkJson(String("{\"schema\":1,\"name\":\"Den cu\",\"ssid\":\"OldHome\",\"password\":\"old-secret\",\"apPassword\":\"short\"}"));
 NetworkStorage oldBad;NetworkConfig unchanged;assert(!oldBad.begin(unchanged)&&!oldBad.blocked&&unchanged.ssid.length()==0);

 // CRC damage permits default provisioning and rewriting a valid config.
 fakeNVS.data["deco-network/config"][5]^=1;
 NetworkStorage damaged;NetworkConfig defaults;
 assert(!damaged.begin(defaults)&&!damaged.blocked&&defaults.hostname==String("den-deco")&&defaults.ssid.length()==0);
 assert(damaged.save(n));NetworkStorage repaired;NetworkConfig repairedConfig;
 assert(repaired.begin(repairedConfig)&&repairedConfig.ssid==n.ssid);

 // Authentic future schemas cannot be overwritten, even for provisioning.
 putNetworkJson(String("{\"schema\":3,\"future\":true}"));
 const auto futureBytes=fakeNVS.data["deco-network/config"];
 NetworkStorage future;NetworkConfig futureConfig;
 assert(!future.begin(futureConfig)&&future.blocked&&!future.save(n));
 assert(fakeNVS.data["deco-network/config"]==futureBytes);
 putNetworkJson(String("{\"schema\":4294967295,\"future\":true}"));
 NetworkStorage distantFuture;NetworkConfig distantConfig;
 assert(!distantFuture.begin(distantConfig)&&distantFuture.blocked&&!distantFuture.save(n));

 fakeNVS=FakeNVS{};NetworkStorage valid;NetworkConfig open;assert(valid.begin(open));
 open.ssid="Guest";open.password="";assert(valid.save(open));
 NetworkStorage readOpen;NetworkConfig guest;assert(readOpen.begin(guest)&&guest.ssid==open.ssid&&guest.password.length()==0);
 const auto validBytes=fakeNVS.data["deco-network/config"];
 for(const char* badHost:{"","-lamp","lamp-","lamp.local","Lamp","two lamps"}){
  NetworkConfig bad=open;bad.hostname=badHost;assert(!valid.save(bad));
  assert(fakeNVS.data["deco-network/config"]==validBytes);
 }
 NetworkConfig tooLong=open;
 tooLong.hostname="abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvw";assert(!valid.save(tooLong));
 tooLong=open;tooLong.name="";assert(!valid.save(tooLong));
 tooLong=open;tooLong.name="123456789012345678901234567890123456789012345678901";assert(!valid.save(tooLong));
 tooLong=open;tooLong.ssid="123456789012345678901234567890123";assert(!valid.save(tooLong));
 tooLong=open;tooLong.password="12345678901234567890123456789012345678901234567890123456789012345";assert(!valid.save(tooLong));
 assert(fakeNVS.data["deco-network/config"]==validBytes);
 // Reader validation also prevents a CRC-valid unsupported hostname loading.
 putNetworkJson(String("{\"schema\":2,\"name\":\"Den\",\"ssid\":\"Home\",\"password\":\"secret123\",\"hostname\":\"bad.local\"}"));
 NetworkStorage badRead;NetworkConfig badReadConfig;assert(!badRead.begin(badReadConfig)&&!badRead.blocked);
 assert(badRead.save(open));
 fakeNVS.failWrite=true;open.hostname="den-khach";
 assert(!badRead.save(open));fakeNVS.failWrite=false;
 NetworkStorage afterFailed;NetworkConfig previous;assert(afterFailed.begin(previous)&&previous.hostname==String("den-deco"));
 fakeNVS.torn=true;assert(!badRead.save(open));fakeNVS.torn=false;
 NetworkStorage torn;NetworkConfig tornConfig;assert(!torn.begin(tornConfig)&&!torn.blocked);assert(torn.save(open));
 fakeNVS=FakeNVS{};fakeNVS.failOpen=true;
 NetworkStorage unavailable;NetworkConfig missing;assert(!unavailable.begin(missing)&&!unavailable.save(missing)&&!unavailable.blocked);
 fakeNVS=FakeNVS{};
}
int main(){
 Settings s;Storage a;a.begin(s);s.power=true;a.changed(s,0);a.tick(s,9999);assert(a.writes==0&&a.dirty);s.light.brightness=20;a.changed(s,6000);a.tick(s,10000);assert(a.writes==0);a.tick(s,16000);assert(a.writes==1&&!a.dirty);
 Settings restored;Storage b;b.begin(restored);assert(restored==s);
 s.light.brightness=30;b.changed(s,20000);fakeNVS.failWrite=true;b.tick(s,30000);assert(b.dirty&&b.error.length()>0&&b.writes==0);fakeNVS.failWrite=false;b.tick(s,34999);assert(b.writes==0);b.tick(s,35000);assert(b.writes==1&&!b.dirty);
 Settings newest;Storage c;c.begin(newest);assert(newest==s);
 s.light.brightness=40;c.changed(s,40000);fakeNVS.torn=true;c.tick(s,50000);assert(c.dirty);fakeNVS.torn=false;Settings fallback;Storage d;d.begin(fallback);assert(fallback.light.brightness==30);
 // Return to the saved state cancels a pending write.
 Settings original=fallback;fallback.light.brightness=31;d.changed(fallback,60000);fallback=original;d.changed(fallback,61000);d.tick(fallback,90000);assert(!d.dirty&&d.writes==0);
 // Invalid schema does not overwrite future data.
 auto& bytes=fakeNVS.data["deco-light/slot-b"];assert(bytes.size()>16);bytes[4]=LIGHT_SCHEMA_VERSION+1;Settings future;Storage e;e.begin(future);assert(e.blocked&&!future.power);future.power=true;e.changed(future,0);assert(!e.saveNow(future));
 fakeNVS=FakeNVS{};Settings invalid;String text=encodeSettings(invalid);Settings roundtrip;assert(decodeSettings(text,roundtrip)&&roundtrip==invalid);assert(!decodeSettings(String("{}"),roundtrip));
 testLightUpgrade();testNetworkStorage();
 std::cout<<"PASS: delayed save/recovery, legacy mode/settings migration, mixed light schemas/new favorites, torn writes, future protection, network migration/hostname/CRC repair\n";
}
