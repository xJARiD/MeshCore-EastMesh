#pragma once

class NetworkStateProvider {
public:
  virtual ~NetworkStateProvider() = default;
  virtual bool isWifiConnected() const = 0;
  virtual bool hasTimeSync() const = 0;
  // True while the fallback setup access point is up (room server builds only).
  virtual bool isSetupApActive() const { return false; }
};
