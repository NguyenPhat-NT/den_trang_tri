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
