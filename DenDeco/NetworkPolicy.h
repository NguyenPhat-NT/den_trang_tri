#pragma once
#include "Config.h"
#include <stdint.h>

namespace Deco {

// One DNS label, deliberately limited to lower-case ASCII for mDNS/WiFi use.
inline bool validHostname(const char* hostname) {
  if (!hostname || !hostname[0] || hostname[0]=='-') return false;
  uint8_t length=0;
  while (hostname[length]) {
    if (length>=48) return false;
    const char c=hostname[length];
    if (!((c>='a' && c<='z') || (c>='0' && c<='9') || c=='-')) return false;
    ++length;
  }
  return hostname[length-1]!='-';
}

// A cached status or DHCP address alone is not proof of a usable connection.
// interfaceReady is the SDK's current connected/IP readiness; associated is
// the radio's current AP association. Keep both checks when core APIs differ.
inline bool homeConnectionOnline(bool configured,bool startPending,
                                 bool interfaceReady,bool associated,
                                 uint32_t ipv4) {
  return configured && !startPending && interfaceReady && associated && ipv4!=0;
}

// The caller cancels the driver attempt before arming this gate. Arduino's
// cached WL_CONNECTED can outlive an AUTH_EXPIRE event, so the input here is
// physical association (esp_wifi_sta_get_ap_info), never that cached status.
// Idle drivers may emit no disconnect event: a finite settle period still
// permits startup, while a driver that remains associated times out safely.
class ConnectionStartGate {
 public:
  enum Decision {Wait,Start,TimedOut};
  bool pending=false;
  uint32_t startedAt=0,generation=0;

  void begin(uint32_t now,uint32_t disconnectGeneration){
    pending=true;startedAt=now;generation=disconnectGeneration;
  }
  void cancel(){pending=false;}
  Decision update(uint32_t now,bool associated,uint32_t disconnectGeneration){
    if(!pending)return Wait;
    uint32_t elapsed=now-startedAt;
    uint32_t settle=disconnectGeneration!=generation?50u:150u;
    if(!associated&&elapsed>=settle){pending=false;return Start;}
    if(associated&&elapsed>=1500u){pending=false;return TimedOut;}
    return Wait;
  }
};

// Hardware-independent timing policy. `online` must include a valid DHCP IP,
// not merely association with an access point. OpenAP can be repeated safely;
// the caller owns the idempotent AP operation and actual WiFi begin/retry.
class ConnectionPolicy {
 public:
  enum Phase { Unconfigured, Connecting, Connected, Failed };
  enum Action { None, OpenAP, Retry };
  Phase phase=Unconfigured;
  uint32_t startedAt=0;

  void begin(uint32_t now, bool configured) {
    configured_=configured;
    phase=configured ? Connecting : Unconfigured;
    startedAt=now;
  }

  Action update(uint32_t now, bool online, bool scanning) {
    if (online) {
      phase=Connected;
      return None;
    }
    if (!configured_) {
      phase=Unconfigured;
      return OpenAP;
    }
    switch (phase) {
      case Connected:
        phase=Connecting;
        startedAt=now;
        return None;
      case Connecting:
        if (uint32_t(now-startedAt)>=Config::WIFI_FALLBACK_MS) {
          phase=Failed;
          startedAt=now;
          return OpenAP;
        }
        return None;
      case Failed:
        if (!scanning && uint32_t(now-startedAt)>=Config::WIFI_FALLBACK_MS) {
          phase=Connecting;
          startedAt=now;
          return Retry;
        }
        return None;
      case Unconfigured:
      default:
        return OpenAP;
    }
  }

 private:
  bool configured_=false;
};

} // namespace Deco
