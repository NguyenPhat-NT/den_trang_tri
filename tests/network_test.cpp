#include "../DenDeco/NetworkPolicy.h"
#include <cassert>
#include <iostream>
#include <string>

using Deco::ConnectionPolicy;

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
  testHostnames();
  testUnconfigured();
  testTimeoutAndRetry();
  testScanDefersRetry();
  testConnectedAndRecovery();
  testRollover();
  std::cout << "network policy tests passed\n";
}
