#include "NetworkService.h"

#include <helpers/TxtDataHelpers.h>
#include <string.h>
#include <time.h>

#if defined(ESP_PLATFORM)
  #include <WiFi.h>
  #include <esp_sntp.h>
#endif

namespace {

#if defined(ESP_PLATFORM)
constexpr unsigned long kWifiRetryMillis = 15000;
constexpr unsigned long kWifiConnectTimeoutMillis = 45000;
constexpr unsigned long kWifiChannelHintTimeoutMillis = 7000;
constexpr time_t kMinSaneEpoch = 1735689600;  // 2025-01-01T00:00:00Z
constexpr size_t kNtpServerMaxLen = 64;

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
    : _fs(nullptr), _prefs{}, _wifi_started(false), _sntp_started(false), _have_time_sync(false), _last_wifi_status(-1), _last_wifi_attempt(0) {
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
  if (_wifi_started) {
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
  }
#endif
  _wifi_started = false;
  _sntp_started = false;
  _have_time_sync = false;
  _last_wifi_status = -1;
  _last_wifi_attempt = 0;
}

void NetworkService::loop(bool network_required) {
#if defined(ESP_PLATFORM)
  ensureWifi(network_required);
  updateTimeSync();
#else
  (void)network_required;
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
             "> ssid:%s status:%s code:%d state:%s ip:%s channel:%d rssi:%d quality:%d%% signal:%s",
             _prefs.wifi_ssid, status, static_cast<int>(wifi_status), state, WiFi.localIP().toString().c_str(),
             WiFi.channel(), rssi_dbm, getWifiQualityPercent(rssi_dbm), getWifiQualityLabel(rssi_dbm));
  } else {
    snprintf(reply, reply_size, "> ssid:%s status:%s code:%d state:%s", _prefs.wifi_ssid[0] ? _prefs.wifi_ssid : "-",
             status, static_cast<int>(wifi_status), state);
  }
#else
  snprintf(reply, reply_size, "> wifi:unsupported");
#endif
}

void NetworkService::reconnectWifi() {
#if defined(ESP_PLATFORM)
  if (_wifi_started) {
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
  }
#endif
  _wifi_started = false;
  _sntp_started = false;
  _have_time_sync = false;
  _last_wifi_attempt = 0;
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
    if (_wifi_started) {
      WiFi.disconnect(true, true);
      WiFi.mode(WIFI_OFF);
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
      WiFi.disconnect(false, false);
      WiFi.mode(WIFI_OFF);
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

  if (!_wifi_started) {
    WiFi.mode(WIFI_STA);
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
#endif
