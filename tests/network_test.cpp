#include "../DenDeco/NetworkPolicy.h"
#include <cassert>
#include <iostream>
#include <string>

using Deco::ConnectionPolicy;
using Deco::ConnectionStartGate;

static void testConnectionStartGate(){
  ConnectionStartGate gate;
  assert(!gate.pending&&gate.update(1000,false,0)==ConnectionStartGate::Wait);
  // A cold/idle driver has no association and may emit no disconnect event.
  // Even if Arduino's cached status were still WL_CONNECTED, it must not
  // prevent the physical, settled driver from starting the new attempt.
  gate.begin(100,7);
  assert(gate.pending&&gate.startedAt==100&&gate.generation==7);
  assert(gate.update(149,false,7)==ConnectionStartGate::Wait);
  assert(gate.update(249,false,7)==ConnectionStartGate::Wait);
  assert(gate.update(250,false,7)==ConnectionStartGate::Start&&!gate.pending);
  assert(gate.update(251,false,8)==ConnectionStartGate::Wait);
  assert(gate.update(10000,false,9)==ConnectionStartGate::Wait);

  // An observed disconnect acknowledgement shortens settling to 50 ms,
  // but neither an acknowledgement nor a stale status replaces association.
  gate.begin(1000,8);
  assert(gate.update(1000,false,9)==ConnectionStartGate::Wait);
  assert(gate.update(1049,false,9)==ConnectionStartGate::Wait);
  assert(gate.update(1050,false,9)==ConnectionStartGate::Start);
  gate.begin(2000,10);
  assert(gate.update(2050,true,11)==ConnectionStartGate::Wait);
  assert(gate.update(2150,true,11)==ConnectionStartGate::Wait);
  assert(gate.update(3499,true,11)==ConnectionStartGate::Wait);
  assert(gate.update(3500,true,11)==ConnectionStartGate::TimedOut&&!gate.pending);
  assert(gate.update(3501,false,11)==ConnectionStartGate::Wait);
  assert(gate.update(5000,true,12)==ConnectionStartGate::Wait);
  // A late physical release is still allowed to start, including exactly
  // at the timeout threshold: TimedOut only applies while associated.
  gate.begin(6000,20);
  assert(gate.update(7499,false,20)==ConnectionStartGate::Start);
  gate.begin(8000,21);
  assert(gate.update(9500,false,21)==ConnectionStartGate::Start);
  gate.begin(10000,22);gate.cancel();
  assert(!gate.pending&&gate.update(20000,false,23)==ConnectionStartGate::Wait);
  gate.begin(21000,24);
  assert(gate.update(21150,false,24)==ConnectionStartGate::Start);

  // millis() and the event-generation counter wrap independently.
  const uint32_t rollover=0xffffffe0u;
  gate.begin(rollover,0xffffffffu);
  assert(gate.update(rollover+49u,false,0)==ConnectionStartGate::Wait);
  assert(gate.update(rollover+50u,false,0)==ConnectionStartGate::Start);
  gate.begin(rollover,0);
  assert(gate.update(rollover+149u,false,0)==ConnectionStartGate::Wait);
  assert(gate.update(rollover+150u,false,0)==ConnectionStartGate::Start);
  gate.begin(rollover,0xffffffffu);
  assert(gate.update(rollover+1499u,true,0)==ConnectionStartGate::Wait);
  assert(gate.update(rollover+1500u,true,0)==ConnectionStartGate::TimedOut);
}

static void testHostnames() {
  assert(Deco::validHostname("den-deco"));
  assert(Deco::validHostname("a"));
  assert(Deco::validHostname("0"));
  assert(Deco::validHostname("a--b-123"));
  assert(Deco::validHostname(std::string(48,'a').c_str()));
  assert(!Deco::validHostname(std::string(49,'a').c_str()));
  const char* invalid[]={nullptr,"","-","-den","den-","Den-deco",
    "den deco","den_deco","den.deco","den/deco","den:deco",
    "den\tdeco","den\ndeco","d\xc3\xa8n-deco"};
  for (const char* hostname:invalid) assert(!Deco::validHostname(hostname));
}

static void testUnconfigured() {
  ConnectionPolicy policy;
  policy.begin(100,false);
  assert(policy.phase==ConnectionPolicy::Unconfigured && policy.startedAt==100);
  assert(policy.update(100,false,false)==ConnectionPolicy::OpenAP);
  assert(policy.update(100000,false,true)==ConnectionPolicy::OpenAP);
  assert(policy.phase==ConnectionPolicy::Unconfigured);
  // An online result has priority, even if credentials were just removed.
  assert(policy.update(100001,true,false)==ConnectionPolicy::None);
  assert(policy.phase==ConnectionPolicy::Connected);
  assert(policy.update(100002,false,false)==ConnectionPolicy::OpenAP);
  assert(policy.phase==ConnectionPolicy::Unconfigured);
}

static void testTimeoutAndRetry() {
  ConnectionPolicy policy;
  const uint32_t start=100, wait=Config::WIFI_FALLBACK_MS;
  policy.begin(start,true);
  assert(policy.phase==ConnectionPolicy::Connecting && policy.startedAt==start);
  // Association without a DHCP address passes online=false and must time out.
  assert(policy.update(start+wait-1,false,false)==ConnectionPolicy::None);
  assert(policy.phase==ConnectionPolicy::Connecting);
  assert(policy.update(start+wait,false,false)==ConnectionPolicy::OpenAP);
  assert(policy.phase==ConnectionPolicy::Failed && policy.startedAt==start+wait);
  assert(policy.update(start+wait+1,false,false)==ConnectionPolicy::None);
  assert(policy.update(start+2*wait-1,false,false)==ConnectionPolicy::None);
  assert(policy.update(start+2*wait,false,false)==ConnectionPolicy::Retry);
  assert(policy.phase==ConnectionPolicy::Connecting && policy.startedAt==start+2*wait);
  assert(policy.update(start+2*wait+1,false,false)==ConnectionPolicy::None);
  assert(policy.update(start+3*wait,false,false)==ConnectionPolicy::OpenAP);
}

static void testScanDefersRetry() {
  ConnectionPolicy policy;
  const uint32_t wait=Config::WIFI_FALLBACK_MS;
  policy.begin(0,true);
  // Scanning defers retries, not the initial fallback AP deadline.
  assert(policy.update(wait,false,true)==ConnectionPolicy::OpenAP);
  assert(policy.update(2*wait,false,true)==ConnectionPolicy::None);
  assert(policy.update(2*wait+1000,false,true)==ConnectionPolicy::None);
  assert(policy.phase==ConnectionPolicy::Failed && policy.startedAt==wait);
  assert(policy.update(2*wait+1001,false,false)==ConnectionPolicy::Retry);
  assert(policy.phase==ConnectionPolicy::Connecting);
  assert(policy.startedAt==2*wait+1001);
}

static void testConnectedAndRecovery() {
  ConnectionPolicy policy;
  const uint32_t wait=Config::WIFI_FALLBACK_MS;
  policy.begin(0,true);
  assert(policy.update(100,true,false)==ConnectionPolicy::None);
  assert(policy.phase==ConnectionPolicy::Connected);
  assert(policy.update(wait*4,true,false)==ConnectionPolicy::None);
  assert(policy.phase==ConnectionPolicy::Connected);
  const uint32_t lost=wait*4+1;
  assert(policy.update(lost,false,false)==ConnectionPolicy::None);
  assert(policy.phase==ConnectionPolicy::Connecting && policy.startedAt==lost);
  // The caller starts reconnect here, which begins the attempt again.
  policy.begin(lost,true);
  assert(policy.update(lost+wait-1,false,false)==ConnectionPolicy::None);
  assert(policy.update(lost+wait,false,false)==ConnectionPolicy::OpenAP);
  assert(policy.update(lost+wait+100,true,true)==ConnectionPolicy::None);
  assert(policy.phase==ConnectionPolicy::Connected);
  assert(policy.update(lost+3*wait,true,false)==ConnectionPolicy::None);
  assert(policy.phase==ConnectionPolicy::Connected);
  // Explicit configuration changes begin a fresh attempt and discard a
  // previous failed timer; missing credentials select the AP immediately.
  policy.begin(500000,true);
  assert(policy.update(500001,false,false)==ConnectionPolicy::None);
  policy.begin(500002,false);
  assert(policy.update(500003,false,false)==ConnectionPolicy::OpenAP);
}

static void testCurrentConnectionReadiness() {
  const uint32_t cachedIP=0xc0a80164u, wait=Config::WIFI_FALLBACK_MS;
  assert(Deco::homeConnectionOnline(true,false,true,true,cachedIP));
  assert(!Deco::homeConnectionOnline(false,false,true,true,cachedIP));
  assert(!Deco::homeConnectionOnline(true,true,true,true,cachedIP));
  assert(!Deco::homeConnectionOnline(true,false,true,true,0));
  // Arduino AUTH_EXPIRE can retain WL_CONNECTED and localIP() can retain
  // the previous DHCP address, while its current connected/IP bits are clear.
  assert(!Deco::homeConnectionOnline(true,false,false,false,cachedIP));
  // An old status on cores whose isConnected() checks status only must still
  // fail when the radio no longer reports association with an access point.
  assert(!Deco::homeConnectionOnline(true,false,true,false,cachedIP));
  // Association alone does not mean DHCP has completed on the current link.
  assert(!Deco::homeConnectionOnline(true,false,false,true,cachedIP));

  ConnectionPolicy policy;
  policy.begin(0,true);
  assert(policy.update(100,Deco::homeConnectionOnline(true,false,true,true,cachedIP),false)
         ==ConnectionPolicy::None);
  assert(policy.phase==ConnectionPolicy::Connected);
  const uint32_t lost=1000;
  // Cached IP stays nonzero for the complete failed reconnect interval.
  assert(policy.update(lost,Deco::homeConnectionOnline(true,false,false,false,cachedIP),false)
         ==ConnectionPolicy::None);
  assert(policy.phase==ConnectionPolicy::Connecting && policy.startedAt==lost);
  assert(policy.update(lost+wait-1,Deco::homeConnectionOnline(true,false,false,false,cachedIP),false)
         ==ConnectionPolicy::None);
  assert(policy.update(lost+wait,Deco::homeConnectionOnline(true,false,false,false,cachedIP),false)
         ==ConnectionPolicy::OpenAP);
  assert(policy.phase==ConnectionPolicy::Failed);
  // The same fallback guarantee holds with stale status-based readiness on
  // older cores, because the physical association input remains false.
  assert(policy.update(lost+2*wait,Deco::homeConnectionOnline(true,false,true,false,cachedIP),false)
         ==ConnectionPolicy::Retry);
  assert(policy.update(lost+3*wait,Deco::homeConnectionOnline(true,false,true,false,cachedIP),false)
         ==ConnectionPolicy::OpenAP);
  assert(policy.update(lost+3*wait+1,Deco::homeConnectionOnline(true,false,true,true,cachedIP),false)
         ==ConnectionPolicy::None);
  assert(policy.phase==ConnectionPolicy::Connected);
}

static void testRollover() {
  ConnectionPolicy policy;
  const uint32_t start=0xfffff000u, wait=Config::WIFI_FALLBACK_MS;
  policy.begin(start,true);
  assert(policy.update(start+wait-1,false,false)==ConnectionPolicy::None);
  assert(policy.update(start+wait,false,false)==ConnectionPolicy::OpenAP);
  assert(policy.update(start+2*wait-1,false,false)==ConnectionPolicy::None);
  assert(policy.update(start+2*wait,false,false)==ConnectionPolicy::Retry);
  assert(policy.update(start+2*wait+100,true,false)==ConnectionPolicy::None);
  assert(policy.phase==ConnectionPolicy::Connected);
}

int main() {
  testConnectionStartGate();
  testHostnames();
  testUnconfigured();
  testTimeoutAndRetry();
  testScanDefersRetry();
  testConnectedAndRecovery();
  testCurrentConnectionReadiness();
  testRollover();
  std::cout << "network startup gate and existing connection policy tests passed\n";
}
