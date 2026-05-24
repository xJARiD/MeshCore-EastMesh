#ifdef ESP_PLATFORM

#include "ESP32Board.h"

#if defined(ADMIN_PASSWORD) && !defined(DISABLE_WIFI_OTA)   // Repeater or Room Server only
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

#include <SPIFFS.h>

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

  sprintf(reply, "Started: http://%s/update", ota_ip.toString().c_str());
  MESH_DEBUG_PRINTLN("startOTAUpdate: %s", reply);

  static char id_buf[60];
  snprintf(id_buf, sizeof(id_buf), "%s (%s)", id, getManufacturerName());
  static char home_buf[90];
  snprintf(home_buf, sizeof(home_buf), "<H2>Hi! I am a MeshCore Repeater. ID: %s</H2>", id);

  static AsyncWebServer* server = nullptr;
  if (server != nullptr) {
    delete server;
    server = nullptr;
  }
  server = new AsyncWebServer(80);

  server->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", home_buf);
  });
  server->on("/log", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/packet_log", "text/plain");
  });

  AsyncElegantOTA.setID(id_buf);
  AsyncElegantOTA.begin(server);    // Start ElegantOTA
  server->begin();

  return true;
}

#else
bool ESP32Board::startOTAUpdate(const char* id, char reply[]) {
  return false; // not supported
}
#endif

#endif
