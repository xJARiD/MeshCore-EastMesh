#pragma once

#include <Arduino.h>
#include <helpers/IdentityStore.h>
#include <helpers/NetworkStateProvider.h>

#if defined(ESP_PLATFORM)
  #include <WiFi.h>

  #include <atomic>
#endif

#include "NetworkPrefs.h"

class NetworkService : public NetworkStateProvider {
public:
  NetworkService();

  void begin(FILESYSTEM* fs,
             uint8_t legacy_wifi_powersave = 0,
             const char* legacy_wifi_ssid = nullptr,
             const char* legacy_wifi_pwd = nullptr);
  void end();
  void loop(bool network_required);

  bool setWifiSSID(const char* ssid);
  bool setWifiPassword(const char* pwd);
  const char* getWifiSSID() const { return _prefs.wifi_ssid; }
  bool setWifiPowerSave(const char* mode);
  const char* getWifiPowerSave() const;
  bool setNtpServer(uint8_t index, const char* server);
  const char* getNtpServer(uint8_t index) const;
  void formatWifiStatusReply(char* reply, size_t reply_size) const;
  void reconnectWifi();
  void forceReconnect();
  // Gateway watchdog state, as reported by `get wifi.status` (gw:ok|lost wd:<n>).
  bool isGatewayReachable() const;
  uint16_t getWatchdogReconnectCount() const;

  bool isWifiConnected() const override;
  bool hasTimeSync() const override { return _have_time_sync; }

private:
#if defined(ESP_PLATFORM)
  static wifi_ps_type_t toEspPowerSave(uint8_t mode);
  static const char* getPowerSaveLabel(uint8_t mode);
  void ensureWifi(bool network_required);
  void updateTimeSync();
  void restartTimeSync();
  void updateConnectivityWatchdog();
  static void watchdogProbeCallback(void* arg);
#endif
  static bool isValidNtpServer(const char* server);
  bool savePrefs();

  FILESYSTEM* _fs;
  NetworkPrefs _prefs;
  bool _wifi_started;
  bool _sntp_started;
  bool _have_time_sync;
  int _last_wifi_status;
  unsigned long _last_wifi_attempt;
#if defined(ESP_PLATFORM)
  // Connectivity watchdog: _wd_gateway_seen and _wd_probe_pending are shared with
  // the lwIP tcpip thread; _wd_gateway_ip is only written while no probe is pending.
  std::atomic<bool> _wd_gateway_seen;
  std::atomic<bool> _wd_probe_pending;
  uint32_t _wd_gateway_ip;
  bool _wd_was_connected;
  unsigned long _wd_last_gateway_ok;
  unsigned long _wd_last_probe;
  uint8_t _wd_backoff_shift;
  uint16_t _wd_reconnect_count;
#endif
};
