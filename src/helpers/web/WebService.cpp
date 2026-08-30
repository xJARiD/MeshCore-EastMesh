#include "WebService.h"

#if defined(ESP_PLATFORM) && WITH_WEB_PANEL
  #include <WiFi.h>
#endif

namespace {
// Retry a heap-deferred start at this cadence rather than every loop tick.
constexpr unsigned long kWebStartRetryMillis = 15000;
// Self-heal: only restart a starved server when nobody has used it for this long,
// and no more often than this.
constexpr unsigned long kWebHealQuietMillis = 60000;
constexpr unsigned long kWebHealMinIntervalMillis = 5UL * 60UL * 1000UL;
constexpr unsigned long kWebHealCheckMillis = 10000;
}  // namespace

WebService::WebService()
    : _fs(nullptr), _prefs{}, _runner(nullptr), _network(nullptr), _suspended_for_ota(false), _last_start_attempt_ms(0),
      _last_heal_ms(0), _last_heal_check_ms(0) {
  WebPrefsStore::setDefaults(_prefs);
}

void WebService::begin(FILESYSTEM* fs) {
  _fs = fs;
  WebPrefsStore::load(_fs, _prefs);
}

void WebService::end() {
#if defined(ESP_PLATFORM) && WITH_WEB_PANEL
  _panel.stop();
#endif
}

void WebService::suspendForOTA() {
#if defined(ESP_PLATFORM) && WITH_WEB_PANEL
  _suspended_for_ota = true;
  _panel.stop();
#endif
}

void WebService::prepareForOTAStart() {
#if defined(ESP_PLATFORM) && WITH_WEB_PANEL
  _suspended_for_ota = true;
  _panel.stopRedirectServer();
#endif
}

void WebService::loop() {
#if defined(ESP_PLATFORM) && WITH_WEB_PANEL
  ensureWebServer();
  const unsigned long now_ms = millis();
  if (_panel.isRunning() && _panel.shouldAutoLock(now_ms)) {
    _panel.lockSession();
  }
  healIfStarved(now_ms);
#endif
}

void WebService::setCommandRunner(WebPanelCommandRunner* runner) {
  _runner = runner;
  _panel.setCommandRunner(runner);
}

bool WebService::savePrefs() {
  return WebPrefsStore::save(_fs, _prefs);
}

bool WebService::setWebEnabled(bool enabled) {
  _prefs.web_enabled = enabled ? 1 : 0;
  if (!enabled) {
    _suspended_for_ota = false;
  }
  bool ok = savePrefs();
#if defined(ESP_PLATFORM) && WITH_WEB_PANEL
  if (_prefs.web_enabled != 0 && !_suspended_for_ota) {
    _last_start_attempt_ms = 0;  // manual enable bypasses the retry backoff
    ensureWebServer();
  } else {
    _panel.stop();
  }
#endif
  return ok;
}

bool WebService::setWebStatsEnabled(bool enabled) {
  _prefs.web_stats_enabled = enabled ? 1 : 0;
  return savePrefs();
}

void WebService::formatWebStatusReply(char* reply, size_t reply_size) const {
#if defined(ESP_PLATFORM) && WITH_WEB_PANEL
  if (_runner == nullptr || _prefs.web_enabled == 0) {
    snprintf(reply, reply_size, "> web:off");
    return;
  }

  if (_suspended_for_ota) {
    snprintf(reply, reply_size, "> web:suspended ota");
    return;
  }

  if (!_panel.isRunning() || _network == nullptr || !_network->isWifiConnected()) {
    snprintf(reply, reply_size, "> web:down heals:%u deferred:%u", _panel.restartCount(), _panel.startDeferredCount());
    return;
  }

  snprintf(reply, reply_size, "> web:up url:https://%s/ auth:%s heals:%u deferred:%u",
           WiFi.localIP().toString().c_str(), _panel.hasSessionToken() ? "unlocked" : "locked",
           _panel.restartCount(), _panel.startDeferredCount());
#else
  snprintf(reply, reply_size, "> web:unsupported");
#endif
}

#if defined(ESP_PLATFORM) && WITH_WEB_PANEL
void WebService::ensureWebServer() {
  if (_suspended_for_ota) {
    if (_panel.isRunning()) {
      _panel.stopRedirectServer();
    }
    return;
  }
  if (_runner == nullptr || _prefs.web_enabled == 0) {
    _panel.stop();
    return;
  }
  if (_network == nullptr || !_network->isWifiConnected()) {
    _panel.stop(false);
    return;
  }
  if (_panel.isRunning()) {
    return;
  }
  const unsigned long now_ms = millis();
  if (_last_start_attempt_ms != 0 && now_ms - _last_start_attempt_ms < kWebStartRetryMillis) {
    return;
  }
  _last_start_attempt_ms = now_ms;
  _panel.start();
}

void WebService::healIfStarved(unsigned long now_ms) {
  if (!_panel.isRunning() || now_ms - _last_heal_check_ms < kWebHealCheckMillis) {
    return;
  }
  _last_heal_check_ms = now_ms;
  if (!_panel.isIdle(now_ms, kWebHealQuietMillis) || !WebPanelServer::isHeapStarved()) {
    return;
  }
  if (_last_heal_ms != 0 && now_ms - _last_heal_ms < kWebHealMinIntervalMillis) {
    return;
  }
  _last_heal_ms = now_ms;
  _panel.noteRestart();
  Serial.printf("[WEB] heap starved (largest internal block too small for TLS), restarting web panel (count=%u)\n",
                _panel.restartCount());
  _panel.stop(false);
  _last_start_attempt_ms = 0;  // let ensureWebServer() retry immediately (subject to the heap gate)
}
#endif
