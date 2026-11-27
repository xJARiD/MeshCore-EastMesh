#include "NetworkService.h"

#include <helpers/TxtDataHelpers.h>
#include <string.h>
#include <time.h>

#if defined(ESP_PLATFORM)
  #include <WiFi.h>
  #include <esp_netif.h>
  #include <esp_netif_net_stack.h>
  #include <esp_sntp.h>
  #include <lwip/etharp.h>
  #include <lwip/netif.h>
  #include <lwip/tcpip.h>
#endif

namespace {

#if defined(ESP_PLATFORM)
constexpr unsigned long kWifiRetryMillis = 15000;
constexpr unsigned long kWifiConnectTimeoutMillis = 45000;
constexpr unsigned long kWifiChannelHintTimeoutMillis = 7000;
constexpr unsigned long kWatchdogProbeMillis = 30000;
constexpr unsigned long kWatchdogTimeoutMillis = 180000;
constexpr uint8_t kWatchdogMaxBackoffShift = 4;  // 180s .. 48min between forced reconnects
constexpr time_t kMinSaneEpoch = 1735689600;  // 2025-01-01T00:00:00Z
constexpr size_t kNtpServerMaxLen = 64;
// Fallback setup AP: offered once Wi-Fi has been down this long (immediately when no
// SSID is configured), and kept for at most this long after Wi-Fi rejoins while a
// client is still attached, so the person who just set Wi-Fi can read the new IP.
constexpr unsigned long kSetupApFallbackMillis = 60000;
constexpr unsigned long kSetupApLingerMillis = 5UL * 60UL * 1000UL;
constexpr unsigned long kSetupApRetryMillis = 30000;

bool isValidWifiChannel(uint8_t channel) {
  return channel >= 1 && channel <= 14;
}

int getWifiQualityPercent(int rssi_dbm) {
  if (rssi_dbm <= -100) {
    return 0;
  }
  if (rssi_dbm >= -50) {
    return 100;
  }
  return 2 * (rssi_dbm + 100);
}

const char* getWifiQualityLabel(int rssi_dbm) {
  if (rssi_dbm >= -60) {
    return "excellent";
  }
  if (rssi_dbm >= -67) {
    return "good";
  }
  if (rssi_dbm >= -75) {
    return "fair";
  }
  return "poor";
}
#endif

}  // namespace

NetworkService::NetworkService()
    : _fs(nullptr), _prefs{}, _wifi_started(false), _sntp_started(false), _have_time_sync(false), _last_wifi_status(-1), _last_wifi_attempt(0),
      _setup_ap_ssid{0}, _setup_ap_active(false) {
#if defined(ESP_PLATFORM)
  _sta_paused_for_ap = false;
  _sta_down_since = 0;
  _sta_up_since = 0;
  _setup_ap_last_attempt = 0;
  _wd_gateway_seen = false;
  _wd_probe_pending = false;
  _wd_gateway_ip = 0;
  _wd_was_connected = false;
  _wd_last_gateway_ok = 0;
  _wd_last_probe = 0;
  _wd_backoff_shift = 0;
  _wd_reconnect_count = 0;
#endif
  NetworkPrefsStore::setDefaults(_prefs);
}

void NetworkService::begin(FILESYSTEM* fs,
                           uint8_t legacy_wifi_powersave,
                           const char* legacy_wifi_ssid,
                           const char* legacy_wifi_pwd) {
  _fs = fs;
  NetworkPrefsStore::load(_fs, _prefs, legacy_wifi_powersave, legacy_wifi_ssid, legacy_wifi_pwd);
#if defined(ESP_PLATFORM)
  Serial.printf("[BOOT] wifi prefs powersave=%s channel=%u ssid=%s\n",
                getPowerSaveLabel(_prefs.wifi_powersave),
                _prefs.wifi_channel,
                _prefs.wifi_ssid[0] ? _prefs.wifi_ssid : "-");
#endif
}

void NetworkService::end() {
#if defined(ESP_PLATFORM)
  if (_wifi_started || _setup_ap_active) {
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
  }
  _setup_ap_active = false;
  _sta_paused_for_ap = false;
#endif
  _wifi_started = false;
  _sntp_started = false;
  _have_time_sync = false;
  _last_wifi_status = -1;
  _last_wifi_attempt = 0;
}

void NetworkService::loop(bool network_required, bool setup_ap_allowed) {
#if defined(ESP_PLATFORM)
  ensureWifi(network_required);
  updateSetupAp(network_required && setup_ap_allowed);
  updateTimeSync();
  updateConnectivityWatchdog();
#else
  (void)network_required;
  (void)setup_ap_allowed;
#endif
}

void NetworkService::setSetupApSsid(const char* ssid) {
  StrHelper::strncpy(_setup_ap_ssid, ssid != nullptr ? ssid : "", sizeof(_setup_ap_ssid));
}

bool NetworkService::setSetupApEnabled(bool enabled) {
  _prefs.setup_ap_off = enabled ? 0 : 1;
  return savePrefs();  // an active AP is taken down on the next loop()
}

void NetworkService::formatSetupApReply(char* reply, size_t reply_size) const {
#if defined(ESP_PLATFORM)
  if (_setup_ap_active) {
    snprintf(reply, reply_size, "> ap:%s state:up ssid:%s ip:%s clients:%u", isSetupApEnabled() ? "auto" : "off",
             _setup_ap_ssid, WiFi.softAPIP().toString().c_str(), static_cast<unsigned>(WiFi.softAPgetStationNum()));
  } else {
    snprintf(reply, reply_size, "> ap:%s state:down ssid:%s", isSetupApEnabled() ? "auto" : "off",
             _setup_ap_ssid[0] ? _setup_ap_ssid : "-");
  }
#else
  snprintf(reply, reply_size, "> ap:unsupported");
#endif
}

bool NetworkService::savePrefs() {
  return NetworkPrefsStore::save(_fs, _prefs);
}

bool NetworkService::setWifiSSID(const char* ssid) {
  if (ssid == nullptr) {
    return false;
  }
  StrHelper::strncpy(_prefs.wifi_ssid, ssid, sizeof(_prefs.wifi_ssid));
#if defined(ESP_PLATFORM)
  _prefs.wifi_channel = 0;
#endif
  bool ok = savePrefs();
  reconnectWifi();
  return ok;
}

bool NetworkService::setWifiPassword(const char* pwd) {
  if (pwd == nullptr) {
    return false;
  }
  StrHelper::strncpy(_prefs.wifi_pwd, pwd, sizeof(_prefs.wifi_pwd));
#if defined(ESP_PLATFORM)
  _prefs.wifi_channel = 0;
#endif
  bool ok = savePrefs();
  reconnectWifi();
  return ok;
}

bool NetworkService::isValidNtpServer(const char* server) {
  if (server == nullptr || server[0] == 0) {
    return false;
  }
  size_t len = 0;
  while (server[len] != 0) {
    const char c = server[len];
    if (c <= ' ' || c == ',' || c == '\x7F') {
      return false;
    }
    len++;
    if (len >= kNtpServerMaxLen) {
      return false;
    }
  }
  return true;
}

bool NetworkService::setNtpServer(uint8_t index, const char* server) {
  if (!isValidNtpServer(server)) {
    return false;
  }

  char* target = nullptr;
  switch (index) {
    case 1:
      target = _prefs.ntp_server1;
      break;
    case 2:
      target = _prefs.ntp_server2;
      break;
    case 3:
      target = _prefs.ntp_server3;
      break;
    default:
      return false;
  }

  if (strcmp(target, server) == 0) {
    return true;
  }

  StrHelper::strncpy(target, server, sizeof(_prefs.ntp_server1));
  const bool ok = savePrefs();
#if defined(ESP_PLATFORM)
  restartTimeSync();
#endif
  return ok;
}

const char* NetworkService::getNtpServer(uint8_t index) const {
  switch (index) {
    case 1:
      return _prefs.ntp_server1;
    case 2:
      return _prefs.ntp_server2;
    case 3:
      return _prefs.ntp_server3;
    default:
      return "";
  }
}

bool NetworkService::setWifiPowerSave(const char* mode) {
  if (mode == nullptr) {
    return false;
  }

  char normalized[8];
  size_t len = 0;
  while (*mode == ' ' || *mode == '\t') {
    mode++;
  }
  while (len < sizeof(normalized) - 1) {
    char c = mode[len];
    if (c == 0 || c == '\r' || c == '\n' || c == ' ' || c == '\t') {
      break;
    }
    normalized[len] = c;
    len++;
  }
  normalized[len] = 0;

  uint8_t next_mode;
  if (strcmp(normalized, "none") == 0) {
    next_mode = 0;
  } else if (strcmp(normalized, "min") == 0) {
    next_mode = 1;
  } else if (strcmp(normalized, "max") == 0) {
    next_mode = 2;
  } else {
    return false;
  }

  if (_prefs.wifi_powersave == next_mode) {
    return true;
  }

  _prefs.wifi_powersave = next_mode;
  bool ok = savePrefs();
#if defined(ESP_PLATFORM)
  if (_wifi_started) {
    WiFi.setSleep(toEspPowerSave(_prefs.wifi_powersave));
  }
#endif
  return ok;
}

const char* NetworkService::getWifiPowerSave() const {
#if defined(ESP_PLATFORM)
  return getPowerSaveLabel(_prefs.wifi_powersave);
#else
  return "unsupported";
#endif
}

void NetworkService::formatWifiStatusReply(char* reply, size_t reply_size) const {
#if defined(ESP_PLATFORM)
  const char* status = "disconnected";
  const char* state = "disconnected";
  wl_status_t wifi_status = WiFi.status();
  if (_prefs.wifi_ssid[0] == 0) {
    status = "unconfigured";
    state = "unconfigured";
  } else if (wifi_status == WL_CONNECTED) {
    status = "connected";
    state = "connected";
  } else if (_wifi_started) {
    status = "connecting";
  }

  switch (wifi_status) {
    case WL_IDLE_STATUS:
      state = "idle";
      break;
    case WL_NO_SSID_AVAIL:
      state = "no_ssid";
      break;
    case WL_SCAN_COMPLETED:
      state = "scan_completed";
      break;
    case WL_CONNECTED:
      state = "connected";
      break;
    case WL_CONNECT_FAILED:
      state = "connect_failed";
      break;
    case WL_CONNECTION_LOST:
      state = "connection_lost";
      break;
    case WL_DISCONNECTED:
      state = "disconnected";
      break;
    default:
      state = "unknown";
      break;
  }

  if (wifi_status == WL_CONNECTED) {
    const int rssi_dbm = WiFi.RSSI();
    snprintf(reply, reply_size,
             "> ssid:%s status:%s code:%d state:%s ip:%s channel:%d rssi:%d quality:%d%% signal:%s gw:%s wd:%u",
             _prefs.wifi_ssid, status, static_cast<int>(wifi_status), state, WiFi.localIP().toString().c_str(),
             WiFi.channel(), rssi_dbm, getWifiQualityPercent(rssi_dbm), getWifiQualityLabel(rssi_dbm),
             isGatewayReachable() ? "ok" : "lost", _wd_reconnect_count);
  } else {
    int len = snprintf(reply, reply_size, "> ssid:%s status:%s code:%d state:%s", _prefs.wifi_ssid[0] ? _prefs.wifi_ssid : "-",
                       status, static_cast<int>(wifi_status), state);
    if (_setup_ap_active && len > 0 && static_cast<size_t>(len) < reply_size) {
      snprintf(reply + len, reply_size - len, " ap:%s", WiFi.softAPIP().toString().c_str());
    }
  }
#else
  snprintf(reply, reply_size, "> wifi:unsupported");
#endif
}

void NetworkService::reconnectWifi() {
#if defined(ESP_PLATFORM)
  if (_wifi_started) {
    WiFi.disconnect(true, true);
    if (!_setup_ap_active) {  // with the setup AP up, drop only the station so its client stays attached
      WiFi.mode(WIFI_OFF);
    }
  }
  _wd_was_connected = false;
  _sta_paused_for_ap = false;  // new credentials or an explicit reconnect always get a try
#endif
  _wifi_started = false;
  _sntp_started = false;
  _have_time_sync = false;
  _last_wifi_attempt = 0;
}

bool NetworkService::isGatewayReachable() const {
#if defined(ESP_PLATFORM)
  return millis() - _wd_last_gateway_ok < (kWatchdogProbeMillis * 3);
#else
  return false;
#endif
}

uint16_t NetworkService::getWatchdogReconnectCount() const {
#if defined(ESP_PLATFORM)
  return _wd_reconnect_count;
#else
  return 0;
#endif
}

void NetworkService::forceReconnect() {
#if defined(ESP_PLATFORM)
  // Clear the channel hint (RAM only) so the retry does a full scan and can land
  // on a different AP; the hint is re-learned and persisted on the next connect.
  _prefs.wifi_channel = 0;
#endif
  reconnectWifi();
}

void NetworkService::restartTimeSync() {
  _sntp_started = false;
  _have_time_sync = false;
}

bool NetworkService::isWifiConnected() const {
#if defined(ESP_PLATFORM)
  return _wifi_started && WiFi.status() == WL_CONNECTED;
#else
  return false;
#endif
}

#if defined(ESP_PLATFORM)
wifi_ps_type_t NetworkService::toEspPowerSave(uint8_t mode) {
  switch (mode) {
    case 1:
      return WIFI_PS_MIN_MODEM;
    case 2:
      return WIFI_PS_MAX_MODEM;
    default:
      return WIFI_PS_NONE;
  }
}

const char* NetworkService::getPowerSaveLabel(uint8_t mode) {
  switch (mode) {
    case 1:
      return "min";
    case 2:
      return "max";
    default:
      return "none";
  }
}

void NetworkService::ensureWifi(bool network_required) {
  if (!network_required) {
    if (_wifi_started || _setup_ap_active) {
      WiFi.disconnect(true, true);
      WiFi.mode(WIFI_OFF);
      _setup_ap_active = false;
      _sta_paused_for_ap = false;
      _wifi_started = false;
      _sntp_started = false;
      _have_time_sync = false;
      _last_wifi_status = -1;
    }
    return;
  }

  if (_prefs.wifi_ssid[0] == 0) {
    reconnectWifi();
    return;
  }

  wl_status_t status = WiFi.status();
  if (static_cast<int>(status) != _last_wifi_status) {
    _last_wifi_status = static_cast<int>(status);
    if (status == WL_CONNECTED) {
      const int connected_channel = WiFi.channel();
      Serial.printf("[BOOT] wifi connected t=%lu ip=%s rssi=%d channel=%d\n",
                    static_cast<unsigned long>(millis()),
                    WiFi.localIP().toString().c_str(),
                    WiFi.RSSI(),
                    connected_channel);
      if (connected_channel > 0 && connected_channel <= 14 && _prefs.wifi_channel != connected_channel) {
        _prefs.wifi_channel = static_cast<uint8_t>(connected_channel);
        Serial.printf("[BOOT] wifi learned channel=%u save=%s\n",
                      _prefs.wifi_channel,
                      savePrefs() ? "ok" : "failed");
      }
    }
  }

  if (status == WL_CONNECTED) {
    return;
  }

  unsigned long now_ms = millis();
  if (_wifi_started) {
    if (isValidWifiChannel(_prefs.wifi_channel) &&
        _last_wifi_attempt != 0 &&
        now_ms - _last_wifi_attempt >= kWifiChannelHintTimeoutMillis) {
      Serial.printf("[BOOT] wifi channel hint timeout t=%lu channel=%u\n",
                    static_cast<unsigned long>(millis()),
                    _prefs.wifi_channel);
      _prefs.wifi_channel = 0;
      savePrefs();
      WiFi.disconnect(false, false);
      _last_wifi_attempt = 0;
    }
    if (_last_wifi_attempt != 0 && now_ms - _last_wifi_attempt < kWifiConnectTimeoutMillis) {
      return;
    }
    if (_last_wifi_attempt != 0) {
      Serial.printf("[BOOT] wifi timeout t=%lu code=%d retry\n",
                    static_cast<unsigned long>(millis()),
                    static_cast<int>(status));
      if (_setup_ap_active) {
        WiFi.disconnect(true, false);  // station off only; keeps the setup AP up
        // A setup client is attached: stop the scan/retry cycle (it hops the AP's
        // channel and drops the client) until they leave or change the Wi-Fi settings.
        _sta_paused_for_ap = setupApHasClients();
      } else {
        WiFi.disconnect(false, false);
        WiFi.mode(WIFI_OFF);
      }
      delay(100);
      _wifi_started = false;
      _sntp_started = false;
      _have_time_sync = false;
      _last_wifi_status = -1;
    }
    if (now_ms - _last_wifi_attempt < kWifiRetryMillis) {
      return;
    }
  }

  if (_sta_paused_for_ap) {
    if (setupApHasClients()) {
      return;
    }
    _sta_paused_for_ap = false;
  }

  if (!_wifi_started) {
    WiFi.mode(_setup_ap_active ? WIFI_AP_STA : WIFI_STA);
    WiFi.setSleep(toEspPowerSave(_prefs.wifi_powersave));
    _wifi_started = true;
    Serial.printf("[BOOT] wifi start t=%lu\n", static_cast<unsigned long>(millis()));
  }

  _last_wifi_attempt = now_ms;
  if (isValidWifiChannel(_prefs.wifi_channel)) {
    WiFi.begin(_prefs.wifi_ssid, _prefs.wifi_pwd, _prefs.wifi_channel);
    Serial.printf("[BOOT] wifi begin t=%lu channel=%u\n", static_cast<unsigned long>(millis()), _prefs.wifi_channel);
  } else {
    WiFi.begin(_prefs.wifi_ssid, _prefs.wifi_pwd);
    Serial.printf("[BOOT] wifi begin t=%lu channel=scan\n", static_cast<unsigned long>(millis()));
  }
}

bool NetworkService::setupApHasClients() const {
  return _setup_ap_active && WiFi.softAPgetStationNum() > 0;
}

void NetworkService::updateSetupAp(bool allowed) {
  const unsigned long now_ms = millis();
  const bool sta_up = _wifi_started && WiFi.status() == WL_CONNECTED;
  if (sta_up) {
    _sta_down_since = 0;
    if (_sta_up_since == 0) {
      _sta_up_since = now_ms != 0 ? now_ms : 1;
    }
  } else {
    _sta_up_since = 0;
    if (_sta_down_since == 0) {
      _sta_down_since = now_ms != 0 ? now_ms : 1;
    }
  }

  bool want = false;
  if (allowed && isSetupApEnabled() && _setup_ap_ssid[0] != 0) {
    if (_prefs.wifi_ssid[0] == 0) {
      want = true;
    } else if (!sta_up) {
      want = _setup_ap_active || now_ms - _sta_down_since >= kSetupApFallbackMillis;
    } else {
      want = _setup_ap_active && setupApHasClients() && now_ms - _sta_up_since < kSetupApLingerMillis;
    }
  }

  if (want && !_setup_ap_active) {
    if (_setup_ap_last_attempt != 0 && now_ms - _setup_ap_last_attempt < kSetupApRetryMillis) {
      return;
    }
    _setup_ap_last_attempt = now_ms != 0 ? now_ms : 1;
    WiFi.mode(_wifi_started ? WIFI_AP_STA : WIFI_AP);
    if (WiFi.softAP(_setup_ap_ssid)) {
      _setup_ap_active = true;
      Serial.printf("[BOOT] setup ap up t=%lu ssid=%s ip=%s\n", static_cast<unsigned long>(millis()), _setup_ap_ssid,
                    WiFi.softAPIP().toString().c_str());
    } else {
      Serial.printf("[BOOT] setup ap start failed t=%lu\n", static_cast<unsigned long>(millis()));
      if (!_wifi_started) {
        WiFi.mode(WIFI_OFF);
      }
    }
  } else if (!want && _setup_ap_active) {
    WiFi.softAPdisconnect(true);
    if (!_wifi_started) {
      WiFi.mode(WIFI_OFF);
    }
    _setup_ap_active = false;
    _sta_paused_for_ap = false;
    _setup_ap_last_attempt = 0;
    Serial.printf("[BOOT] setup ap down t=%lu\n", static_cast<unsigned long>(millis()));
  }
}

void NetworkService::updateTimeSync() {
  bool prev_have_time_sync = _have_time_sync;
  if (!_wifi_started || WiFi.status() != WL_CONNECTED) {
    _have_time_sync = false;
    return;
  }

  if (!_sntp_started) {
    configTzTime("UTC0", _prefs.ntp_server1, _prefs.ntp_server2, _prefs.ntp_server3);
    _sntp_started = true;
  }

  sntp_sync_status_t sync_status = sntp_get_sync_status();
  time_t now = time(nullptr);
  bool sane_time = now >= kMinSaneEpoch;
  bool sync_ready = sync_status == SNTP_SYNC_STATUS_COMPLETED || sync_status == SNTP_SYNC_STATUS_IN_PROGRESS;
  _have_time_sync = sane_time && (sync_ready || prev_have_time_sync);
}

// Runs in the lwIP tcpip thread (posted via tcpip_callback), where raw etharp
// calls are safe without core locking.
void NetworkService::watchdogProbeCallback(void* arg) {
  NetworkService* self = static_cast<NetworkService*>(arg);
  esp_netif_t* esp_nif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
  struct netif* nif = esp_nif ? static_cast<struct netif*>(esp_netif_get_netif_impl(esp_nif)) : nullptr;
  if (nif != nullptr && netif_is_up(nif)) {
    ip4_addr_t gw;
    gw.addr = self->_wd_gateway_ip;
    struct eth_addr* eth_ret = nullptr;
    const ip4_addr_t* ip_ret = nullptr;
    if (etharp_find_addr(nif, &gw, &eth_ret, &ip_ret) >= 0) {
      self->_wd_gateway_seen = true;
    }
    etharp_request(nif, &gw);
  }
  self->_wd_probe_pending = false;
}

void NetworkService::updateConnectivityWatchdog() {
  if (!_wifi_started || WiFi.status() != WL_CONNECTED) {
    _wd_was_connected = false;
    return;
  }

  const unsigned long now_ms = millis();
  if (!_wd_was_connected) {
    // Fresh association: give the gateway a full window before judging it.
    _wd_was_connected = true;
    _wd_last_gateway_ok = now_ms;
    _wd_last_probe = 0;
    _wd_gateway_seen = false;
  }

  if (_wd_gateway_seen.exchange(false)) {
    _wd_last_gateway_ok = now_ms;
    _wd_backoff_shift = 0;
  }

  const uint32_t gateway_ip = static_cast<uint32_t>(WiFi.gatewayIP());
  if (gateway_ip == 0) {
    // No gateway (e.g. static IP without one) - nothing meaningful to probe.
    _wd_last_gateway_ok = now_ms;
    return;
  }

  if (now_ms - _wd_last_probe >= kWatchdogProbeMillis && !_wd_probe_pending.load()) {
    _wd_last_probe = now_ms;
    _wd_gateway_ip = gateway_ip;
    _wd_probe_pending = true;
    if (tcpip_callback(watchdogProbeCallback, this) != ERR_OK) {
      _wd_probe_pending = false;
    }
  }

  const unsigned long timeout = kWatchdogTimeoutMillis << _wd_backoff_shift;
  if (now_ms - _wd_last_gateway_ok >= timeout) {
    _wd_reconnect_count++;
    if (_wd_backoff_shift < kWatchdogMaxBackoffShift) {
      _wd_backoff_shift++;
    }
    Serial.printf("[WDOG] gateway unreachable for %lus, forcing wifi reconnect (count=%u)\n",
                  (now_ms - _wd_last_gateway_ok) / 1000, _wd_reconnect_count);
    forceReconnect();
  }
}
#endif
