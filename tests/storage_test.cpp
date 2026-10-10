#include "../DenDeco/Storage.h"
#include <cassert>
#include <iostream>
FakeNVS fakeNVS;
using namespace Deco;
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
 auto& bytes=fakeNVS.data["deco-light/slot-b"];assert(bytes.size()>16);bytes[4]=2;Settings future;Storage e;e.begin(future);assert(e.blocked&&!future.power);future.power=true;e.changed(future,0);assert(!e.saveNow(future));
 fakeNVS=FakeNVS{};Settings invalid;String text=encodeSettings(invalid);Settings roundtrip;assert(decodeSettings(text,roundtrip)&&roundtrip==invalid);assert(!decodeSettings(String("{}"),roundtrip));
 testNetworkStorage();
 std::cout<<"PASS: delayed/debounced save, recovery, failed writes and retry, torn slot fallback, no-op save, future schema, network migration/hostname/CRC repair\n";
}
