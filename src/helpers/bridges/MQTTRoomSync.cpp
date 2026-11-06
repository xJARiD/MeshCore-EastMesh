#include "MQTTRoomSync.h"

#if defined(WITH_ROOM_SYNC) && defined(WITH_MQTT_BRIDGE) && defined(ESP_PLATFORM)

#include <AES.h>
#include <Arduino.h>
#include <SHA256.h>
#include <string.h>

#ifndef ROOM_SYNC_DEBUG
  #define ROOM_SYNC_DEBUG 0
#endif

#if ROOM_SYNC_DEBUG
  #define ROOM_SYNC_LOG(fmt, ...) Serial.printf("[ROOMSYNC] " fmt "\n", ##__VA_ARGS__)
#else
  #define ROOM_SYNC_LOG(...) do { } while (0)
#endif

namespace {
constexpr const char* kTopicPrefix = "meshcore/rooms/";
constexpr const char* kKeyLabel = "eastmesh-room-sync-v1";
constexpr uint8_t kMagic0 = 'E';
constexpr uint8_t kMagic1 = 'R';
// Posts older than this (by the origin room's clock) are treated as replays.
constexpr uint32_t kMaxPostAgeSecs = 24UL * 60UL * 60UL;
// Below this the local clock has clearly never been set, so skip the age check.
constexpr uint32_t kMinValidEpoch = 1735689600UL;  // 1 Jan 2025

bool constantTimeEquals(const uint8_t* a, const uint8_t* b, size_t len) {
  uint8_t diff = 0;
  for (size_t i = 0; i < len; i++) {
    diff |= a[i] ^ b[i];
  }
  return diff == 0;
}
}  // namespace

MQTTRoomSync::MQTTRoomSync(NodePrefs* prefs, MQTTBridge* bridge, mesh::RTCClock* rtc)
    : _prefs(prefs), _bridge(bridge), _rtc(rtc), _self_id(nullptr), _rng(nullptr), _handler(nullptr),
      _queue_lock(portMUX_INITIALIZER_UNLOCKED), _queue_head(0), _queue_count(0), _dropped(0), _seen_next(0),
      _tx_count(0), _rx_count(0), _rejected_count(0), _dup_count(0) {
  memset(_seen, 0, sizeof(_seen));
}

void MQTTRoomSync::begin(const mesh::LocalIdentity* self_id, mesh::RNG* rng, RoomSyncHandler* handler) {
  _self_id = self_id;
  _rng = rng;
  _handler = handler;
  if (_bridge != nullptr) {
    _bridge->setTopicListener(this);
  }
}

bool MQTTRoomSync::isConfigured() const {
  return _prefs != nullptr && _prefs->bridge_room_topic[0] != 0;
}

bool MQTTRoomSync::isOnline() const {
  return isConfigured() && _bridge != nullptr && _bridge->isRunning() && _bridge->isConnected();
}

bool MQTTRoomSync::getBridgeTopic(char* buf, size_t buf_size) {
  if (!isConfigured() || buf == nullptr || buf_size == 0) {
    return false;
  }
  int n = snprintf(buf, buf_size, "%s%s", kTopicPrefix, _prefs->bridge_room_topic);
  return n > 0 && static_cast<size_t>(n) < buf_size;
}

void MQTTRoomSync::deriveKey(uint8_t key[32]) const {
  SHA256 sha;
  sha.update(kKeyLabel, strlen(kKeyLabel));
  uint8_t sep = 0;
  sha.update(&sep, 1);
  sha.update(_prefs->bridge_room_topic, strlen(_prefs->bridge_room_topic));
  sha.update(&sep, 1);
  sha.update(_prefs->bridge_secret, strnlen(_prefs->bridge_secret, sizeof(_prefs->bridge_secret)));
  sha.finalize(key, 32);
}

void MQTTRoomSync::ctrCrypt(const uint8_t key[32], const uint8_t nonce[kNonceSize], uint8_t* data, size_t len) {
  AES128 aes;
  aes.setKey(key, 16);
  uint8_t counter[16];
  uint8_t stream[16];
  memcpy(counter, nonce, kNonceSize);
  uint64_t block = 0;
  for (size_t off = 0; off < len; off += 16, block++) {
    for (int i = 0; i < 8; i++) {
      counter[kNonceSize + i] = static_cast<uint8_t>(block >> (56 - 8 * i));
    }
    aes.encryptBlock(stream, counter);
    size_t n = (len - off) < 16 ? (len - off) : 16;
    for (size_t i = 0; i < n; i++) {
      data[off + i] ^= stream[i];
    }
  }
}

void MQTTRoomSync::computeTag(const uint8_t key[32], const uint8_t* data, size_t len, uint8_t tag[kTagSize]) {
  SHA256 sha;
  sha.resetHMAC(key, 32);
  sha.update(data, len);
  sha.finalizeHMAC(key, 32, tag, kTagSize);
}

bool MQTTRoomSync::publishPost(const mesh::Identity& author, uint32_t post_timestamp, const char* text) {
  if (!isOnline() || _self_id == nullptr || _rng == nullptr || text == nullptr) {
    return false;
  }

  size_t text_len = strnlen(text, ROOM_SYNC_MAX_TEXT_LEN);
  if (text_len == 0) {
    return false;
  }

  uint8_t frame[kMaxFrameSize];
  size_t pos = 0;
  frame[pos++] = kMagic0;
  frame[pos++] = kMagic1;
  frame[pos++] = kVersion;
  uint8_t* nonce = &frame[pos];
  _rng->random(nonce, kNonceSize);
  pos += kNonceSize;

  uint8_t* plain = &frame[pos];
  size_t plain_len = 0;
  memcpy(&plain[plain_len], _self_id->pub_key, kOriginSize);
  plain_len += kOriginSize;
  uint8_t* msg_id = &plain[plain_len];
  _rng->random(msg_id, 4);
  plain_len += 4;
  memcpy(&plain[plain_len], author.pub_key, PUB_KEY_SIZE);
  plain_len += PUB_KEY_SIZE;
  memcpy(&plain[plain_len], &post_timestamp, 4);
  plain_len += 4;
  plain[plain_len++] = static_cast<uint8_t>(text_len);
  memcpy(&plain[plain_len], text, text_len);
  plain_len += text_len;

  // Remember our own message so a broker echo is ignored even if the origin check is bypassed.
  seenBefore(_self_id->pub_key, msg_id);

  uint8_t key[32];
  deriveKey(key);
  ctrCrypt(key, nonce, plain, plain_len);
  pos += plain_len;
  computeTag(key, frame, pos, &frame[pos]);
  pos += kTagSize;
  memset(key, 0, sizeof(key));

  if (!_bridge->publishListenerTopic(frame, pos)) {
    ROOM_SYNC_LOG("publish failed");
    return false;
  }
  _tx_count++;
  ROOM_SYNC_LOG("published post len=%u", static_cast<unsigned>(text_len));
  return true;
}

void MQTTRoomSync::onBridgeTopicData(const uint8_t* data, size_t len) {
  if (data == nullptr || len == 0 || len > kMaxFrameSize) {
    _dropped = _dropped + 1;
    return;
  }
  portENTER_CRITICAL(&_queue_lock);
  if (_queue_count >= kQueueSlots) {
    portEXIT_CRITICAL(&_queue_lock);
    _dropped = _dropped + 1;
    return;
  }
  int slot = (_queue_head + _queue_count) % kQueueSlots;
  _queue[slot].len = static_cast<uint16_t>(len);
  memcpy(_queue[slot].data, data, len);
  _queue_count = _queue_count + 1;
  portEXIT_CRITICAL(&_queue_lock);
}

void MQTTRoomSync::loop() {
  for (int i = 0; i < kQueueSlots; i++) {
    Frame frame;
    portENTER_CRITICAL(&_queue_lock);
    if (_queue_count == 0) {
      portEXIT_CRITICAL(&_queue_lock);
      return;
    }
    frame.len = _queue[_queue_head].len;
    memcpy(frame.data, _queue[_queue_head].data, frame.len);
    _queue_head = (_queue_head + 1) % kQueueSlots;
    _queue_count = _queue_count - 1;
    portEXIT_CRITICAL(&_queue_lock);

    processFrame(frame.data, frame.len);
  }
}

bool MQTTRoomSync::seenBefore(const uint8_t* origin, const uint8_t* msg_id) {
  uint8_t hash[4];
  mesh::Utils::sha256(hash, sizeof(hash), origin, kOriginSize, msg_id, 4);
  uint32_t value;
  memcpy(&value, hash, sizeof(value));
  if (value == 0) {
    value = 1;  // 0 marks an empty slot
  }
  for (int i = 0; i < kSeenSlots; i++) {
    if (_seen[i] == value) {
      return true;
    }
  }
  _seen[_seen_next] = value;
  _seen_next = (_seen_next + 1) % kSeenSlots;
  return false;
}

void MQTTRoomSync::processFrame(const uint8_t* data, size_t len) {
  if (!isConfigured() || _handler == nullptr || _self_id == nullptr) {
    return;
  }
  if (len < kHeaderSize + kPlainFixedSize + 1 + kTagSize || data[0] != kMagic0 || data[1] != kMagic1 ||
      data[2] != kVersion) {
    _rejected_count++;
    return;
  }

  uint8_t key[32];
  deriveKey(key);
  uint8_t tag[kTagSize];
  const size_t body_len = len - kTagSize;
  computeTag(key, data, body_len, tag);
  if (!constantTimeEquals(tag, &data[body_len], kTagSize)) {
    memset(key, 0, sizeof(key));
    _rejected_count++;
    ROOM_SYNC_LOG("tag mismatch (topic/secret differ?)");
    return;
  }

  uint8_t plain[kPlainFixedSize + ROOM_SYNC_MAX_TEXT_LEN];
  const size_t plain_len = body_len - kHeaderSize;
  if (plain_len > sizeof(plain)) {
    memset(key, 0, sizeof(key));
    _rejected_count++;
    return;
  }
  memcpy(plain, &data[kHeaderSize], plain_len);
  ctrCrypt(key, &data[3], plain, plain_len);
  memset(key, 0, sizeof(key));

  const uint8_t* origin = &plain[0];
  const uint8_t* msg_id = &plain[kOriginSize];
  if (memcmp(origin, _self_id->pub_key, kOriginSize) == 0) {
    return;  // our own post echoed back by the broker
  }
  if (seenBefore(origin, msg_id)) {
    _dup_count++;
    return;
  }

  mesh::Identity author(&plain[kOriginSize + 4]);
  uint32_t post_timestamp;
  memcpy(&post_timestamp, &plain[kOriginSize + 4 + PUB_KEY_SIZE], 4);
  uint8_t text_len = plain[kOriginSize + 4 + PUB_KEY_SIZE + 4];
  if (text_len == 0 || text_len > ROOM_SYNC_MAX_TEXT_LEN || kPlainFixedSize + text_len != plain_len) {
    _rejected_count++;
    return;
  }

  uint32_t now = _rtc != nullptr ? _rtc->getCurrentTime() : 0;
  if (now >= kMinValidEpoch && post_timestamp >= kMinValidEpoch && post_timestamp + kMaxPostAgeSecs < now) {
    _rejected_count++;
    ROOM_SYNC_LOG("stale post dropped");
    return;
  }

  char text[ROOM_SYNC_MAX_TEXT_LEN + 1];
  memcpy(text, &plain[kPlainFixedSize], text_len);
  text[text_len] = 0;

  _rx_count++;
  _handler->onRoomSyncPost(author, post_timestamp, text);
}

void MQTTRoomSync::formatStatusReply(char* reply, size_t reply_size) const {
  const char* state;
  if (!isConfigured()) {
    state = "no-topic";
  } else if (_bridge == nullptr || !_bridge->isRunning()) {
    state = _bridge != nullptr && !_bridge->hasPeerHost() ? "no-host" : "off";
  } else if (!_bridge->isConnected()) {
    state = "connecting";
  } else {
    state = "online";
  }
  snprintf(reply, reply_size, "> sync:%s topic:%s tx:%lu rx:%lu dup:%lu bad:%lu drop:%lu", state,
           isConfigured() ? _prefs->bridge_room_topic : "-", static_cast<unsigned long>(_tx_count),
           static_cast<unsigned long>(_rx_count), static_cast<unsigned long>(_dup_count),
           static_cast<unsigned long>(_rejected_count), static_cast<unsigned long>(_dropped));
}

#endif
