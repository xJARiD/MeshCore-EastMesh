#pragma once

#include <Arduino.h>
#include <helpers/IdentityStore.h>
#include <helpers/NetworkStateProvider.h>

#include "WebPanelServer.h"
#include "WebPrefs.h"

class WebService {
public:
  WebService();

  void begin(FILESYSTEM* fs);
  void end();
  void loop();
  void suspendForOTA();
  void prepareForOTAStart();

  void setCommandRunner(WebPanelCommandRunner* runner);
  void setNetworkStateProvider(NetworkStateProvider* network) { _network = network; }

  bool setWebEnabled(bool enabled);
  bool isWebEnabled() const { return _prefs.web_enabled != 0; }
  bool setWebStatsEnabled(bool enabled);
  bool isWebStatsEnabled() const { return _prefs.web_stats_enabled != 0; }

  void formatWebStatusReply(char* reply, size_t reply_size) const;
  bool isPanelRunning() const { return _panel.isRunning(); }
  bool isPanelUnlocked() const { return _panel.hasSessionToken(); }

private:
#if defined(ESP_PLATFORM) && WITH_WEB_PANEL
  void ensureWebServer();
  void healIfStarved(unsigned long now_ms);
#endif
  bool savePrefs();

  FILESYSTEM* _fs;
  WebPrefs _prefs;
  WebPanelCommandRunner* _runner;
  NetworkStateProvider* _network;
  WebPanelServer _panel;
  bool _suspended_for_ota;
  unsigned long _last_start_attempt_ms;
  unsigned long _last_heal_ms;
  unsigned long _last_heal_check_ms;
};
