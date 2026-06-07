#include "NetworkPrefs.h"

#include <helpers/TxtDataHelpers.h>
#include <string.h>

#if defined(ESP_PLATFORM)
  #include <Preferences.h>
#endif

namespace {

struct LegacyWebPrefsV1 {
  uint32_t magic;
  uint8_t web_enabled;
  uint8_t web_stats_enabled;
  uint8_t wifi_powersave;
  uint8_t reserved;
  char wifi_ssid[33];
  char wifi_pwd[65];
};

bool loadLegacyWebWifiPrefs(FILESYSTEM* fs, NetworkPrefs& prefs) {
  if (fs == nullptr || !fs->exists("/web_prefs")) {
    return false;
  }

#if defined(RP2040_PLATFORM)
  File file = fs->open("/web_prefs", "r");
#else
  File file = fs->open("/web_prefs");
#endif
  if (!file) {
    return false;
  }

  if (static_cast<size_t>(file.size()) < sizeof(LegacyWebPrefsV1)) {
    file.close();
    return false;
  }

  LegacyWebPrefsV1 legacy{};
  bool ok = file.read(reinterpret_cast<uint8_t*>(&legacy), sizeof(legacy)) == sizeof(legacy);
  file.close();
  if (!ok || legacy.magic != 0x57454250) {
    return false;
  }

  prefs.wifi_powersave = legacy.wifi_powersave <= 2 ? legacy.wifi_powersave : 0;
  StrHelper::strncpy(prefs.wifi_ssid, legacy.wifi_ssid, sizeof(prefs.wifi_ssid));
  StrHelper::strncpy(prefs.wifi_pwd, legacy.wifi_pwd, sizeof(prefs.wifi_pwd));
  return true;
}

#if defined(ESP_PLATFORM)
bool loadNvsNetworkPrefs(NetworkPrefs& prefs) {
  Preferences nvs;
  if (!nvs.begin("eastmesh-net", true)) {
    return false;
  }
  NetworkPrefs stored{};
  const size_t read = nvs.getBytes("prefs", &stored, sizeof(stored));
  nvs.end();
  if (read != sizeof(stored) || stored.magic != NetworkPrefsStore::magicValue()) {
    return false;
  }
  prefs = stored;
  return true;
}

bool saveNvsNetworkPrefs(const NetworkPrefs& prefs) {
  Preferences nvs;
  if (!nvs.begin("eastmesh-net", false)) {
    return false;
  }
  const size_t written = nvs.putBytes("prefs", &prefs, sizeof(prefs));
  nvs.end();
  return written == sizeof(prefs);
}
#else
bool loadNvsNetworkPrefs(NetworkPrefs&) {
  return false;
}

bool saveNvsNetworkPrefs(const NetworkPrefs&) {
  return true;
}
#endif

}  // namespace

void NetworkPrefsStore::setDefaults(NetworkPrefs& prefs) {
  memset(&prefs, 0, sizeof(prefs));
  prefs.magic = kMagic;
  prefs.wifi_powersave = 0;
  prefs.wifi_channel = 0;
}

bool NetworkPrefsStore::load(FILESYSTEM* fs, NetworkPrefs& prefs,
                             uint8_t legacy_wifi_powersave,
                             const char* legacy_wifi_ssid,
                             const char* legacy_wifi_pwd) {
  setDefaults(prefs);
  if (fs == nullptr) {
    loadNvsNetworkPrefs(prefs);
    return false;
  }

  if (!fs->exists(kFilename)) {
    if (loadNvsNetworkPrefs(prefs)) {
      save(fs, prefs);
      return true;
    }
    prefs.wifi_powersave = legacy_wifi_powersave <= 2 ? legacy_wifi_powersave : 0;
    if (legacy_wifi_ssid != nullptr) {
      StrHelper::strncpy(prefs.wifi_ssid, legacy_wifi_ssid, sizeof(prefs.wifi_ssid));
    }
    if (legacy_wifi_pwd != nullptr) {
      StrHelper::strncpy(prefs.wifi_pwd, legacy_wifi_pwd, sizeof(prefs.wifi_pwd));
    }
    loadLegacyWebWifiPrefs(fs, prefs);
    save(fs, prefs);
    return false;
  }

#if defined(RP2040_PLATFORM)
  File file = fs->open(kFilename, "r");
#else
  File file = fs->open(kFilename);
#endif
  if (!file) {
    return false;
  }

  NetworkPrefs persisted{};
  size_t bytes_to_read = min(static_cast<size_t>(file.size()), sizeof(persisted));
  bool ok = bytes_to_read >= sizeof(persisted.magic) &&
            file.read(reinterpret_cast<uint8_t*>(&persisted), bytes_to_read) == bytes_to_read;
  file.close();

  if (!ok || persisted.magic != kMagic) {
    fs->remove(kFilename);
    if (loadNvsNetworkPrefs(prefs)) {
      save(fs, prefs);
      return true;
    }
    save(fs, prefs);
    return false;
  }

  prefs = persisted;
  if (prefs.wifi_powersave > 2) {
    prefs.wifi_powersave = 0;
  }
  if (prefs.wifi_channel < 1 || prefs.wifi_channel > 14) {
    prefs.wifi_channel = 0;
  }
  bool repaired = false;
  if (prefs.wifi_ssid[0] == 0 && legacy_wifi_ssid != nullptr && legacy_wifi_ssid[0] != 0) {
    StrHelper::strncpy(prefs.wifi_ssid, legacy_wifi_ssid, sizeof(prefs.wifi_ssid));
    prefs.wifi_channel = 0;
    repaired = true;
  }
  if (prefs.wifi_pwd[0] == 0 && legacy_wifi_pwd != nullptr && legacy_wifi_pwd[0] != 0) {
    StrHelper::strncpy(prefs.wifi_pwd, legacy_wifi_pwd, sizeof(prefs.wifi_pwd));
    prefs.wifi_channel = 0;
    repaired = true;
  }
  if (repaired) {
    save(fs, prefs);
  } else {
    saveNvsNetworkPrefs(prefs);
  }
  return true;
}

bool NetworkPrefsStore::save(FILESYSTEM* fs, const NetworkPrefs& prefs) {
  const bool nvs_ok = saveNvsNetworkPrefs(prefs);
  if (fs == nullptr) {
    return nvs_ok;
  }
  if (fs->exists(kFilename) && !fs->remove(kFilename)) {
    return nvs_ok;
  }
#if defined(RP2040_PLATFORM)
  File file = fs->open(kFilename, "w");
#else
  File file = fs->open(kFilename, "w", true);
#endif
  if (!file) {
    return nvs_ok;
  }
  bool ok = file.write(reinterpret_cast<const uint8_t*>(&prefs), sizeof(prefs)) == sizeof(prefs);
  file.close();
  return ok && nvs_ok;
}
