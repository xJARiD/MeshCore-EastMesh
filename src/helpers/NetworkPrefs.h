#pragma once

#include <helpers/IdentityStore.h>
#include <stdint.h>

// IMPORTANT: persisted to NVS and LittleFS as a raw byte blob. To keep saved
// WiFi credentials across firmware updates, ONLY ever APPEND new fields to the
// end of this struct, and never reorder, resize, or remove existing fields or
// change `magic`. The loaders copy the overlapping prefix, so appended fields
// load as zero on older saves and are then filled in by the *Defaults helpers.
struct NetworkPrefs {
  uint32_t magic;
  uint8_t wifi_powersave;
  uint8_t wifi_channel;
  uint8_t reserved[2];
  char wifi_ssid[33];
  char wifi_pwd[65];
  char ntp_server1[64];
  char ntp_server2[64];
  char ntp_server3[64];
  // Room-server fallback setup AP: 0 = auto (default, also what older saves load as), 1 = off.
  uint8_t setup_ap_off;
};

class NetworkPrefsStore {
public:
  static void setDefaults(NetworkPrefs& prefs);
  static bool load(FILESYSTEM* fs, NetworkPrefs& prefs,
                   uint8_t legacy_wifi_powersave = 0,
                   const char* legacy_wifi_ssid = nullptr,
                   const char* legacy_wifi_pwd = nullptr);
  static bool save(FILESYSTEM* fs, const NetworkPrefs& prefs);
  static constexpr uint32_t magicValue() { return kMagic; }

private:
  static constexpr uint32_t kMagic = 0x4E455450;
  static constexpr const char* kFilename = "/network_prefs";
};
