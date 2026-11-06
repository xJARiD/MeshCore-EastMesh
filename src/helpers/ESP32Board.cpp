#ifdef ESP_PLATFORM

#include "ESP32Board.h"
#include <target.h>

// The RTC backup of ESP32RTCClock, declared in ESP32Board.h.
RTC_NOINIT_ATTR uint32_t _rtc_backup_time;
RTC_NOINIT_ATTR uint32_t _rtc_backup_magic;

#if defined(ADMIN_PASSWORD) && !defined(DISABLE_WIFI_OTA)   // Repeater or Room Server only
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

#include <SPIFFS.h>

namespace {
AsyncWebServer* ota_server = nullptr;
char ota_id_buf[60];
char ota_home_buf[90];
uint16_t ota_server_port = 0;
constexpr uint8_t kOtaPort80Attempts = 8;
constexpr uint32_t kOtaStartRetryDelayMs = 500;

bool startOTAServerNow(uint16_t port) {
  if (ota_server != nullptr) {
    ota_server->end();
    delete ota_server;
    ota_server = nullptr;
    ota_server_port = 0;
  }

  ota_server = new AsyncWebServer(port);

  ota_server->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", ota_home_buf);
  });
  ota_server->on("/log", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/packet_log", "text/plain");
  });

  AsyncElegantOTA.setID(ota_id_buf);
  AsyncElegantOTA.begin(ota_server);    // Start ElegantOTA
  ota_server->begin();
  if (ota_server->state() != LISTEN) {
    ota_server->end();
    delete ota_server;
    ota_server = nullptr;
    ota_server_port = 0;
    return false;
  }
  ota_server_port = port;
  return true;
}

bool startOTAServerWithFallback() {
  for (uint8_t attempt = 1; attempt <= kOtaPort80Attempts; ++attempt) {
    if (startOTAServerNow(80)) {
      MESH_DEBUG_PRINTLN("OTA server listening on port 80");
      return true;
    }
    MESH_DEBUG_PRINTLN("OTA server port 80 busy, retry %u/%u", attempt, kOtaPort80Attempts);
    delay(kOtaStartRetryDelayMs);
  }

  static const uint16_t fallback_ports[] = {8080, 8081};
  for (uint8_t i = 0; i < sizeof(fallback_ports) / sizeof(fallback_ports[0]); ++i) {
    const uint16_t port = fallback_ports[i];
    if (startOTAServerNow(port)) {
      MESH_DEBUG_PRINTLN("OTA server listening on fallback port %u", port);
      return true;
    }
    MESH_DEBUG_PRINTLN("OTA server fallback port %u busy", port);
  }

  MESH_DEBUG_PRINTLN("OTA server failed to bind any port");
  return false;
}
}

bool ESP32Board::startOTAUpdate(const char* id, char reply[]) {
  inhibit_sleep = true;   // prevent sleep during OTA

  IPAddress ota_ip;
  if (WiFi.status() == WL_CONNECTED) {
    ota_ip = WiFi.localIP();
  } else {
    WiFi.mode(WIFI_AP_STA);
    if (!WiFi.softAP("MeshCore-OTA", NULL)) {
      strcpy(reply, "Error - OTA AP start failed");
      return false;
    }
    ota_ip = WiFi.softAPIP();
  }

  snprintf(ota_id_buf, sizeof(ota_id_buf), "%s (%s)", id, getManufacturerName());
  snprintf(ota_home_buf, sizeof(ota_home_buf), "<H2>Hi! I am a MeshCore Repeater. ID: %s</H2>", id);

  if (!startOTAServerWithFallback()) {
    strcpy(reply, "Error - OTA listener start failed");
    return false;
  }

  if (ota_server_port == 80) {
    sprintf(reply, "Started: http://%s/update", ota_ip.toString().c_str());
  } else {
    sprintf(reply, "Started: http://%s:%u/update", ota_ip.toString().c_str(), ota_server_port);
  }
  MESH_DEBUG_PRINTLN("startOTAUpdate: %s", reply);
  return true;
}

#else
bool ESP32Board::startOTAUpdate(const char* id, char reply[]) {
  return false; // not supported
}
#endif

void ESP32Board::powerOff() {
  enterDeepSleep(0); // Do not wakeup
}

void ESP32Board::enterDeepSleep(uint32_t secs) {
  // Power off the display if any
#ifdef DISPLAY_CLASS
  display.turnOff();
#endif

  // Power off LoRa
  radio_driver.powerOff();

  // Keep LoRa inactive during deepsleep
  digitalWrite(P_LORA_NSS, HIGH);
#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C6)
  gpio_hold_en((gpio_num_t)P_LORA_NSS);
#else
  rtc_gpio_hold_en((gpio_num_t)P_LORA_NSS);
#endif

  // Power off GPS if any
  if (sensors.getLocationProvider() != NULL) {
    sensors.getLocationProvider()->stop();
  }

  // Flush serial buffers
  Serial.flush();
  delay(100);

  // Clear stale wakeup sources to avoid ghost wakeup
  // This is required when Power Management and automatic lightsleep are enabled
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

  if (secs > 0) {
    esp_sleep_enable_timer_wakeup(secs * 1000000ULL);
  }

  // Finally set ESP32 into deepsleep
  esp_deep_sleep_start(); // CPU halts here and never returns!
}
#endif
