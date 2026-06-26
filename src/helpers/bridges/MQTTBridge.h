#pragma once

#include "helpers/bridges/BridgeBase.h"

#ifdef WITH_MQTT_BRIDGE

#if defined(ESP_PLATFORM)
#include <mqtt_client.h>
#endif

/**
 * @brief Bridge implementation using MQTT for bidirectional mesh packet transport
 *
 * Publishes and subscribes on a shared topic at a peer MQTT broker. Uses the same
 * binary framing and XOR encryption as other bridge types for network isolation.
 */
class MQTTBridge : public BridgeBase {
private:
  static MQTTBridge *_instance;

  static void mqttEventHandler(void *handler_args, esp_event_base_t base, int32_t event_id,
                               void *event_data);

  static const char *kBridgeTopic;
  static const size_t MAX_MQTT_PACKET_SIZE = 512;
  static const size_t MAX_PAYLOAD_SIZE = MAX_MQTT_PACKET_SIZE - (BRIDGE_MAGIC_SIZE + BRIDGE_CHECKSUM_SIZE);

  esp_mqtt_client_handle_t _client;
  bool _connected;
  bool _started;
  bool _pending_destroy;
  unsigned long _next_connect_attempt;
  uint8_t _reconnect_failures;
  char _client_id[24];

  void xorCrypt(uint8_t *data, size_t len);
  void destroyClient();
  void scheduleClientDestroy();
  bool ensureClient();
  void handleMqttData(const uint8_t *data, size_t len);
  void onMqttConnected();
  void onMqttDisconnected();

public:
  MQTTBridge(NodePrefs *prefs, mesh::PacketManager *mgr, mesh::RTCClock *rtc);

  void begin() override;
  void end() override;
  void loop() override;
  void onPacketReceived(mesh::Packet *packet) override;
  void sendPacket(mesh::Packet *packet) override;
};

#endif
