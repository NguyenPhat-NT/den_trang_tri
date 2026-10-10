#include "../DenDeco/Storage.h"
#include <cassert>
#include <iostream>
FakeNVS fakeNVS;
using namespace Deco;
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
 NetworkStorage network;NetworkConfig n;assert(network.begin(n));n.ssid="Home";n.password="testing123";n.apPassword="ABCDabcd1234";assert(network.save(n));NetworkStorage network2;NetworkConfig read;assert(network2.begin(read)&&read.password==n.password&&read.ssid==n.ssid);fakeNVS.data["deco-network/config"][5]^=1;NetworkStorage network3;NetworkConfig damaged;assert(!network3.begin(damaged));
 std::cout<<"PASS: delayed/debounced save, recovery, failed writes and retry, torn slot fallback, no-op save, future schema and network CRC\n";
}
