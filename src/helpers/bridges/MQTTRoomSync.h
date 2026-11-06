#pragma once

#include "helpers/bridges/MQTTBridge.h"

#if defined(WITH_ROOM_SYNC) && defined(WITH_MQTT_BRIDGE) && defined(ESP_PLATFORM)

#include <Identity.h>
#include <Utils.h>
#include <freertos/FreeRTOS.h>

#ifndef ROOM_SYNC_MAX_TEXT_LEN
  #define ROOM_SYNC_MAX_TEXT_LEN 151   // matches simple_room_server MAX_POST_TEXT_LEN
#endif

/**
 * EastMesh room-server post sync.
 *
 * Room servers that share a bridge broker, bridge secret and room topic relay each
 * new post to one another over `meshcore/rooms/<topic>`. A receiving room stores the
 * post under the original author and pushes it to its own logged-in clients.
 *
 * Wire format (v1):
 *   'E' 'R' ver(1) nonce(8) ciphertext(n) tag(8)
 * ciphertext is AES-128-CTR over:
 *   origin_prefix(8) msg_id(4) author_pubkey(32) post_timestamp(4) text_len(1) text(text_len)
 * tag is HMAC-SHA256 over header+nonce+ciphertext, truncated to 8 bytes.
 * The key is SHA-256 of a fixed label, the topic and the bridge secret, so only rooms
 * with the same topic and secret can read or inject posts.
 */
class RoomSyncHandler {
public:
  virtual ~RoomSyncHandler() = default;
  virtual void onRoomSyncPost(const mesh::Identity& author, uint32_t post_timestamp, const char* text) = 0;
};

class MQTTRoomSync : public MQTTBridgeTopicListener {
public:
  MQTTRoomSync(NodePrefs* prefs, MQTTBridge* bridge, mesh::RTCClock* rtc);

  void begin(const mesh::LocalIdentity* self_id, mesh::RNG* rng, RoomSyncHandler* handler);
  void loop();

  // Publishes a locally-originated post. Returns false if sync is off or not connected.
  bool publishPost(const mesh::Identity& author, uint32_t post_timestamp, const char* text);

  bool isConfigured() const;
  bool isOnline() const;
  void formatStatusReply(char* reply, size_t reply_size) const;

  // MQTTBridgeTopicListener (runs on the MQTT task)
  bool getBridgeTopic(char* buf, size_t buf_size) override;
  void onBridgeTopicData(const uint8_t* data, size_t len) override;

private:
  static const uint8_t kVersion = 1;
  static const size_t kNonceSize = 8;
  static const size_t kTagSize = 8;
  static const size_t kHeaderSize = 3 + kNonceSize;
  static const size_t kOriginSize = 8;
  static const size_t kPlainFixedSize = kOriginSize + 4 + PUB_KEY_SIZE + 4 + 1;
  static const size_t kMaxFrameSize = kHeaderSize + kPlainFixedSize + ROOM_SYNC_MAX_TEXT_LEN + kTagSize;
  static const int kQueueSlots = 4;
  static const int kSeenSlots = 32;

  struct Frame {
    uint16_t len;
    uint8_t data[kMaxFrameSize];
  };

  NodePrefs* _prefs;
  MQTTBridge* _bridge;
  mesh::RTCClock* _rtc;
  const mesh::LocalIdentity* _self_id;
  mesh::RNG* _rng;
  RoomSyncHandler* _handler;

  // MQTT task -> main loop hand-off
  portMUX_TYPE _queue_lock;
  Frame _queue[kQueueSlots];
  volatile int _queue_head;
  volatile int _queue_count;
  volatile uint32_t _dropped;

  uint32_t _seen[kSeenSlots];
  int _seen_next;

  uint32_t _tx_count;
  uint32_t _rx_count;
  uint32_t _rejected_count;
  uint32_t _dup_count;

  void deriveKey(uint8_t key[32]) const;
  static void ctrCrypt(const uint8_t key[32], const uint8_t nonce[kNonceSize], uint8_t* data, size_t len);
  static void computeTag(const uint8_t key[32], const uint8_t* data, size_t len, uint8_t tag[kTagSize]);
  bool seenBefore(const uint8_t* origin, const uint8_t* msg_id);
  void processFrame(const uint8_t* data, size_t len);
};

#endif
