#include "MQTTBridge.h"

#ifdef WITH_MQTT_BRIDGE

#if defined(ESP_PLATFORM)

#include <Arduino.h>
#include <WiFi.h>
#include <ctype.h>
#include <string.h>

#ifndef BRIDGE_DEBUG
  #define BRIDGE_DEBUG 0
#endif

#if BRIDGE_DEBUG
  #define BRIDGE_DEBUG_PRINTLN(...) Serial.printf(__VA_ARGS__)
#else
  #define BRIDGE_DEBUG_PRINTLN(...) do { } while (0)
#endif

namespace {
constexpr unsigned long kConnectRetryBaseMillis = 10000;
constexpr unsigned long kConnectRetryMaxMillis = 120000;

unsigned long connectRetryDelayMillis(uint8_t failures) {
  unsigned long delay_ms = kConnectRetryBaseMillis;
  if (failures > 0) {
    uint8_t shifts = min<uint8_t>(failures - 1, 3);
    delay_ms <<= shifts;
  }
  if (delay_ms > kConnectRetryMaxMillis) {
    delay_ms = kConnectRetryMaxMillis;
  }
  return delay_ms;
}

bool peerHostConfigured(const NodePrefs *prefs) {
  return prefs != nullptr && prefs->bridge_peer_host[0] != 0;
}

uint16_t peerPort(const NodePrefs *prefs) {
  return prefs->bridge_peer_port != 0 ? prefs->bridge_peer_port : 1883;
}
}  // namespace

MQTTBridge *MQTTBridge::_instance = nullptr;
const char *MQTTBridge::kBridgeTopic = "meshcore/bridge/packets";

MQTTBridge::MQTTBridge(NodePrefs *prefs, mesh::PacketManager *mgr, mesh::RTCClock *rtc)
    : BridgeBase(prefs, mgr, rtc), _client(nullptr), _connected(false), _started(false),
      _pending_destroy(false), _next_connect_attempt(0), _reconnect_failures(0) {
  _instance = this;
  _client_id[0] = 0;
}

void MQTTBridge::xorCrypt(uint8_t *data, size_t len) {
  size_t keyLen = strlen(_prefs->bridge_secret);
  if (keyLen == 0) {
    return;
  }
  for (size_t i = 0; i < len; i++) {
    data[i] ^= _prefs->bridge_secret[i % keyLen];
  }
}

void MQTTBridge::destroyClient() {
  if (_client != nullptr) {
    esp_mqtt_client_stop(_client);
    esp_mqtt_client_destroy(_client);
    _client = nullptr;
  }
  _connected = false;
  _started = false;
  _pending_destroy = false;
}

void MQTTBridge::scheduleClientDestroy() {
  _connected = false;
  _started = false;
  _pending_destroy = true;
}

void MQTTBridge::begin() {
  BRIDGE_DEBUG_PRINTLN("MQTT bridge initializing\n");
  if (!peerHostConfigured(_prefs)) {
    BRIDGE_DEBUG_PRINTLN("MQTT bridge peer host not configured\n");
    return;
  }

  uint8_t mac[6];
  WiFi.macAddress(mac);
  snprintf(_client_id, sizeof(_client_id), "mc-br-%02x%02x%02x", mac[3], mac[4], mac[5]);

  _initialized = true;
  _next_connect_attempt = 0;
  _reconnect_failures = 0;
}

void MQTTBridge::end() {
  BRIDGE_DEBUG_PRINTLN("MQTT bridge stopping\n");
  destroyClient();
  _initialized = false;
}

void MQTTBridge::onMqttConnected() {
  _connected = true;
  _reconnect_failures = 0;
  _next_connect_attempt = 0;
  if (_client != nullptr) {
    esp_mqtt_client_subscribe(_client, kBridgeTopic, 0);
  }
  BRIDGE_DEBUG_PRINTLN("MQTT bridge connected to %s:%u\n", _prefs->bridge_peer_host,
                       static_cast<unsigned>(peerPort(_prefs)));
}

void MQTTBridge::onMqttDisconnected() {
  scheduleClientDestroy();
  _next_connect_attempt = millis() + connectRetryDelayMillis(_reconnect_failures);
  if (_reconnect_failures < 255) {
    ++_reconnect_failures;
  }
}

void MQTTBridge::mqttEventHandler(void *handler_args, esp_event_base_t, int32_t event_id, void *event_data) {
  auto *bridge = static_cast<MQTTBridge *>(handler_args);
  if (bridge == nullptr) {
    return;
  }

  switch (event_id) {
    case MQTT_EVENT_CONNECTED:
      bridge->onMqttConnected();
      break;
    case MQTT_EVENT_DISCONNECTED:
      bridge->onMqttDisconnected();
      break;
    case MQTT_EVENT_ERROR:
      bridge->onMqttDisconnected();
      break;
    case MQTT_EVENT_DATA: {
      auto *event = static_cast<esp_mqtt_event_handle_t>(event_data);
      if (event != nullptr && event->data_len > 0) {
        bridge->handleMqttData(reinterpret_cast<const uint8_t *>(event->data),
                               static_cast<size_t>(event->data_len));
      }
      break;
    }
    default:
      break;
  }
}

bool MQTTBridge::ensureClient() {
  if (_client != nullptr) {
    return true;
  }
  if (!peerHostConfigured(_prefs)) {
    return false;
  }
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  esp_mqtt_client_config_t cfg = {};
#if ESP_IDF_VERSION_MAJOR >= 5
  cfg.broker.address.hostname = _prefs->bridge_peer_host;
  cfg.broker.address.port = peerPort(_prefs);
  cfg.broker.address.transport = MQTT_TRANSPORT_OVER_TCP;
  cfg.credentials.client_id = _client_id;
  if (_prefs->bridge_peer_username[0] != 0) {
    cfg.credentials.username = _prefs->bridge_peer_username;
  }
  if (_prefs->bridge_peer_password[0] != 0) {
    cfg.credentials.authentication.password = _prefs->bridge_peer_password;
  }
  cfg.session.keepalive = 30;
  cfg.network.reconnect_timeout_ms = 10000;
  cfg.network.timeout_ms = 10000;
  cfg.network.disable_auto_reconnect = true;
  cfg.buffer.size = MAX_MQTT_PACKET_SIZE;
  cfg.buffer.out_size = MAX_MQTT_PACKET_SIZE;
#else
  cfg.host = _prefs->bridge_peer_host;
  cfg.port = peerPort(_prefs);
  cfg.transport = MQTT_TRANSPORT_OVER_TCP;
  cfg.client_id = _client_id;
  if (_prefs->bridge_peer_username[0] != 0) {
    cfg.username = _prefs->bridge_peer_username;
  }
  if (_prefs->bridge_peer_password[0] != 0) {
    cfg.password = _prefs->bridge_peer_password;
  }
  cfg.keepalive = 30;
  cfg.buffer_size = MAX_MQTT_PACKET_SIZE;
  cfg.out_buffer_size = MAX_MQTT_PACKET_SIZE;
  cfg.reconnect_timeout_ms = 10000;
  cfg.network_timeout_ms = 10000;
  cfg.disable_auto_reconnect = true;
#endif

  _client = esp_mqtt_client_init(&cfg);
  if (_client == nullptr) {
    BRIDGE_DEBUG_PRINTLN("MQTT bridge client init failed\n");
    return false;
  }

  esp_mqtt_client_register_event(_client, MQTT_EVENT_ANY, &MQTTBridge::mqttEventHandler, this);
  if (esp_mqtt_client_start(_client) != ESP_OK) {
    BRIDGE_DEBUG_PRINTLN("MQTT bridge client start failed\n");
    destroyClient();
    return false;
  }

  _started = true;
  return true;
}

void MQTTBridge::loop() {
  if (!_initialized) {
    return;
  }

  if (_pending_destroy) {
    destroyClient();
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    if (_client != nullptr) {
      destroyClient();
    }
    return;
  }

  if (_client != nullptr) {
    return;
  }

  if (_next_connect_attempt != 0 && millis() < _next_connect_attempt) {
    return;
  }
  ensureClient();
}

void MQTTBridge::handleMqttData(const uint8_t *data, size_t len) {
  if (len < (BRIDGE_MAGIC_SIZE + BRIDGE_CHECKSUM_SIZE)) {
    BRIDGE_DEBUG_PRINTLN("MQTT RX packet too small, len=%u\n", static_cast<unsigned>(len));
    return;
  }

  if (len > MAX_MQTT_PACKET_SIZE) {
    BRIDGE_DEBUG_PRINTLN("MQTT RX packet too large, len=%u\n", static_cast<unsigned>(len));
    return;
  }

  uint16_t received_magic = (data[0] << 8) | data[1];
  if (received_magic != BRIDGE_PACKET_MAGIC) {
    BRIDGE_DEBUG_PRINTLN("MQTT RX invalid magic 0x%04X\n", received_magic);
    return;
  }

  uint8_t decrypted[MAX_MQTT_PACKET_SIZE];
  const size_t encryptedDataLen = len - BRIDGE_MAGIC_SIZE;
  memcpy(decrypted, data + BRIDGE_MAGIC_SIZE, encryptedDataLen);

  xorCrypt(decrypted, encryptedDataLen);

  uint16_t received_checksum = (decrypted[0] << 8) | decrypted[1];
  const size_t payloadLen = encryptedDataLen - BRIDGE_CHECKSUM_SIZE;

  if (!validateChecksum(decrypted + BRIDGE_CHECKSUM_SIZE, payloadLen, received_checksum)) {
    BRIDGE_DEBUG_PRINTLN("MQTT RX checksum mismatch, rcv=0x%04X\n", received_checksum);
    return;
  }

  mesh::Packet *pkt = _mgr->allocNew();
  if (!pkt) {
    return;
  }

  if (pkt->readFrom(decrypted + BRIDGE_CHECKSUM_SIZE, payloadLen)) {
    onPacketReceived(pkt);
  } else {
    _mgr->free(pkt);
  }
}

void MQTTBridge::sendPacket(mesh::Packet *packet) {
  if (!_initialized || !_connected || _client == nullptr || packet == nullptr) {
    return;
  }

  if (_seen_packets.hasSeen(packet)) {
    return;
  }

  uint8_t sizingBuffer[MAX_PAYLOAD_SIZE];
  uint16_t meshPacketLen = packet->writeTo(sizingBuffer);
  if (meshPacketLen > MAX_PAYLOAD_SIZE) {
    BRIDGE_DEBUG_PRINTLN("MQTT TX packet too large (payload=%u, max=%u)\n", meshPacketLen,
                         static_cast<unsigned>(MAX_PAYLOAD_SIZE));
    return;
  }

  uint8_t buffer[MAX_MQTT_PACKET_SIZE];
  buffer[0] = (BRIDGE_PACKET_MAGIC >> 8) & 0xFF;
  buffer[1] = BRIDGE_PACKET_MAGIC & 0xFF;

  const size_t packetOffset = BRIDGE_MAGIC_SIZE + BRIDGE_CHECKSUM_SIZE;
  memcpy(buffer + packetOffset, sizingBuffer, meshPacketLen);

  uint16_t checksum = fletcher16(buffer + packetOffset, meshPacketLen);
  buffer[2] = (checksum >> 8) & 0xFF;
  buffer[3] = checksum & 0xFF;

  xorCrypt(buffer + BRIDGE_MAGIC_SIZE, meshPacketLen + BRIDGE_CHECKSUM_SIZE);

  const size_t totalPacketSize = BRIDGE_MAGIC_SIZE + BRIDGE_CHECKSUM_SIZE + meshPacketLen;
  int msg_id = esp_mqtt_client_publish(_client, kBridgeTopic, reinterpret_cast<const char *>(buffer),
                                       static_cast<int>(totalPacketSize), 0, 0);
  if (msg_id >= 0) {
    BRIDGE_DEBUG_PRINTLN("MQTT TX, len=%u\n", meshPacketLen);
  } else {
    BRIDGE_DEBUG_PRINTLN("MQTT TX FAILED\n");
  }
}

void MQTTBridge::onPacketReceived(mesh::Packet *packet) {
  handleReceivedPacket(packet);
}

#endif  // ESP_PLATFORM

#endif  // WITH_MQTT_BRIDGE
