# Custom CLI Commands

This page covers the EastMesh-specific CLI commands added in this repository.

It does not try to repeat the full upstream MeshCore CLI surface.

## Start Here

If you are doing first-time setup, these are the commands most users need before anything else:

```text
set wifi.ssid <your-ssid>
set wifi.pwd <your-password>
get wifi.status
set mqtt.iata <code>
get mqtt.status
```

For observers with the local web panel, these are also useful:

```text
set web on
get web.status
set web off
```

Use `set web on` while setting up or troubleshooting, then use `set web off` when a fixed observer needs maximum memory headroom.

## Observer Commands

These commands are available on `*_repeater_observer` firmware targets.

No-argument `get` commands must be entered exactly as shown.

### MQTT Status And Routing

- `get mqtt.status`: shows Wi-Fi, NTP, IATA, endpoint status, custom endpoint transport, status publishing state, TX state, and `leaked:<n>` (MQTT clients intentionally left unfreed after a heap-integrity failure at teardown; should stay `0`).
- `get mqtt.statuscfg`: shows whether periodic status messages are enabled as a simple `on` or `off` value. Most users can just use `get mqtt.status`.
- `get mqtt.client_version`: shows the MQTT `client_version` string published by the repeater.
- `get mqtt.client_env`: shows the PlatformIO env used to build the repeater firmware.
- `get mqtt.iata`: shows the IATA/location code used in MQTT topics.
- `set mqtt.iata <code>`: sets the IATA/location code, for example `MEL`.
- `set mqtt.iata UNSET`: marks MQTT IATA as not configured yet. While it is `UNSET`, enabled MQTT brokers stay disconnected until a real code is saved.

### MQTT Identity

- `get mqtt.owner`: shows the configured owner public key.
- `set mqtt.owner <64-hex-char-public-key>`: sets the owner public key used in JWT metadata.
- `mqtt.owner <64-hex-char-public-key>`: shorthand for setting the owner public key.
- `get mqtt.email`: shows the configured owner email.
- `set mqtt.email <email>`: sets the owner email used in JWT metadata.
- `mqtt.email <email>`: shorthand for setting the owner email.

### MQTT Message Controls

- `get mqtt.packets`: shows whether packet messages are published.
- `set mqtt.packets on|off`: enables or disables packet publishing.
- `get mqtt.raw`: shows whether raw packet payloads are published.
- `set mqtt.raw on|off`: enables or disables the separate `raw` MQTT topic.
- `set mqtt.status on|off`: enables or disables periodic MQTT status publishing.
- `get mqtt.tx`: shows whether TX packets are included.
- `set mqtt.tx on|off`: enables or disables TX packet publishing.

### MQTT Endpoints

- `get mqtt.eastmesh-au`
- `set mqtt.eastmesh-au on|off`
- `get mqtt.meshmapper`
- `set mqtt.meshmapper on|off`
- `get mqtt.waev`
- `set mqtt.waev on|off`
- `get mqtt.letsmesh-eu` (retired)
- `set mqtt.letsmesh-eu on|off` (retired)
- `get mqtt.letsmesh-us` (retired)
- `set mqtt.letsmesh-us on|off` (retired)
- `get mqtt.custom`
- `set mqtt.custom on|off`
- `get mqtt.custom.host`
- `set mqtt.custom.host <host>`
- `get mqtt.custom.port`
- `set mqtt.custom.port <port>`
- `get mqtt.custom.transport`: shows `tcp` or `wss`.
- `set mqtt.custom.transport tcp|wss`
- `get mqtt.custom.username`
- `set mqtt.custom.username <username>`
- `get mqtt.custom.password`: shows `set` when a custom password is configured.
- `set mqtt.custom.password <password>`

Notes:

- new observer installs default `mqtt.iata` to `UNSET`
- a maximum of two MQTT brokers can be enabled at once
- `meshmapper` is the curated global broker (`wss://mqtt.meshmapper.net:443/mqtt`); like `eastmesh-au` it uses WSS, verified TLS, and MeshCore JWT auth with the broker host as the token audience
- `waev` is a curated broker (`wss://mqtt.waev.app:443/mqtt`); like `eastmesh-au` it uses WSS, verified TLS, and MeshCore JWT auth with the broker host as the token audience, but enforces a 1-hour auth token lifetime (the firmware mints a shorter-lived token for it automatically)
- `letsmesh-eu` and `letsmesh-us` are retired (LetsMesh is no longer maintained) and off by default; any saved selections are cleared once on upgrade, though the endpoints stay selectable for legacy use
- if `mqtt.iata` is `UNSET`, enabled MQTT brokers will not connect
- custom MQTT uses the configured username and password, not JWT authentication
- custom MQTT defaults to TCP; set `mqtt.custom.transport wss` for MQTT over secure WebSockets using the fixed `/mqtt` websocket path and the ESP-IDF x509 root CA bundle
- `get mqtt.status` reports the two enabled broker slots as `p:<broker>:<state>` (primary) and `s:<broker>:<state>` (secondary), or `-` when a slot is empty; custom appears as `custom:<transport>:<state>` (for example `p:custom:wss:up`); `conn` means connecting and `up` means connected
- custom MQTT uses the same `meshcore/<IATA>/<device>/<leaf>` topics as the curated brokers
- turning off a connected broker publishes a retained MQTT status update with `"status":"offline"` before the client disconnects
- changing `mqtt.iata` away from a configured value also publishes retained offline status to the old status topic, restarts connected broker clients, and reconnects under the new topic path

Legacy dotted aliases are also accepted:

- `mqtt.eastmesh.au`
- `mqtt.letsmesh.eu`
- `mqtt.letsmesh.us`

### Wi-Fi Settings For Observers

- `get wifi.status`: shows SSID, connection state, raw Wi-Fi status code, IP, channel, and signal when connected, plus gateway health (`gw:ok|lost`) and the watchdog reconnect count (`wd:<n>`).
- `get wifi.ssid`: shows the configured Wi-Fi SSID.
- `set wifi.ssid <ssid>`: sets the Wi-Fi SSID.
- `set wifi.pwd <password>`: sets the Wi-Fi password.
- `get wifi.powersaving`: shows the current Wi-Fi power save mode.
- `set wifi.powersaving none|min|max`: sets Wi-Fi power saving mode.
- `wifi reconnect`: drops the current association and rejoins from a full channel scan. Use when a node reports connected but is unreachable over the network.

Observers also run a connectivity watchdog: while Wi-Fi reports connected, the node ARP-probes its gateway every 30 seconds. If the gateway stays silent for 3 minutes (for example, an AP that keeps beaconing after losing its wired uplink), the node forces a full reconnect on its own, backing off up to 48 minutes between attempts while the outage persists. `wd:<n>` in `get wifi.status` counts these forced reconnects since boot.

### NTP Settings For Observers

- `get ntp.server1`: shows the primary NTP server.
- `get ntp.server2`: shows the secondary NTP server.
- `get ntp.server3`: shows the tertiary NTP server.
- `set ntp.server1 <host>`: sets the primary NTP server and restarts time sync.
- `set ntp.server2 <host>`: sets the secondary NTP server and restarts time sync.
- `set ntp.server3 <host>`: sets the tertiary NTP server and restarts time sync.

Default servers are `au.pool.ntp.org`, `time.google.com`, and `time.cloudflare.com`.

### ESP-NOW Bridge Settings For Observer ESP-NOW Builds

These commands are available on `*_repeater_observer_espnow` firmware targets that use the ESP-NOW bridge transport.

Bridge commands are for local ESP-NOW bridge use between nearby repeaters, such as linking repeaters on `Australia (Narrow)` and `Australia (Mid)`. They are not MQTT-over-WAN, VPN, or internet bridge controls.

- `get bridge.channel`: shows the configured ESP-NOW bridge channel.
- `set bridge.channel <channel>`: sets the ESP-NOW bridge channel and restarts the bridge. Use a value from `1` to `14`.
- `get bridge.secret`: shows the configured ESP-NOW bridge secret.
- `set bridge.secret <secret>`: sets the shared ESP-NOW bridge secret and restarts the bridge.

After running `set bridge.channel`, expect the bridge and web panel connection to drop briefly while the radio restarts. On current observer ESP-NOW builds, this can look like the board rebooted.

For `*_repeater_observer_espnow` builds that are connected to Wi-Fi, the ESP-NOW bridge channel must match the active 2.4 GHz Wi-Fi channel:

1. Run `get wifi.status`.
2. Read the `channel:<n>` value from the connected Wi-Fi status.
3. Run `get bridge.channel`.
4. If the values differ, run `set bridge.channel <n>` using the Wi-Fi channel value.
5. Use the same `bridge.channel` and `bridge.secret` on every ESP-NOW bridge node that should talk together.

Example:

```text
> get wifi.status
> ssid:EastMesh-IoT status:connected code:3 state:connected ip:192.168.1.50 channel:6 rssi:-61 quality:78% signal:good gw:ok wd:0
> get bridge.channel
> 1
> set bridge.channel 6
OK
```

### MQTT Bridge Settings For Observer MQTT Bridge Builds

These commands are available on `*_repeater_observer_mqtt_bridge` firmware targets. The bridge relays raw, encrypted mesh packets to other bridge nodes through a peer MQTT broker you run. That broker is separate from the MQTT uplink brokers above.

- `get bridge.enabled`, `set bridge.enabled on|off`
- `get bridge.peer.host`, `set bridge.peer.host <host>`: broker hostname or IP address (no scheme, port, or path).
- `get bridge.peer.port`, `set bridge.peer.port <1-65535>`: default `1883`.
- `get bridge.peer.username`, `set bridge.peer.username <user>`
- `get bridge.peer.password`, `set bridge.peer.password <password>`: `get` only reports `set` or `-`.
- `get bridge.secret`, `set bridge.secret <secret>`: shared secret (up to 15 characters) that encrypts bridged packets. Every bridge node must use the same one.

Notes:

- every bridge node must use the same broker, credentials, and `bridge.secret`
- packets are bridged on the fixed `meshcore/bridge/packets` topic over plain MQTT/TCP. TLS and WebSockets are not supported on the bridge connection
- `*_room_server_mqtt_bridge` room servers use the same topic and join an existing observer MQTT bridge when they share the broker and secret
- the README has a worked example of a [Mosquitto peer broker](https://github.com/xJARiD/MeshCore-EastMesh#peer-mosquitto-broker-lan)

### Web Panel Controls

- `get web`
- `get web.status`: shows whether the local HTTPS panel is available, plus `heals:<n>` (times the panel was restarted because internal heap was too fragmented for a TLS handshake) and `deferred:<n>` (start attempts postponed until heap headroom recovered).
- `get web.stats.status`: shows whether the dedicated stats page and history subsystem are enabled, whether recent history is active, whether PSRAM-backed history is available, and whether the SD-backed archive is mounted. When enabled, the history capture now covers supported environment telemetry too, not just the original battery/radio series. GPS-active boards also record per-minute satellites samples for the `/stats` history view. If archive access drops while stats remain enabled, the repeater retries the SD mount periodically.
- At boot, archive-backed stats restore uses bounded reads of the latest summary, events, and neighbour snapshots so malformed or unexpectedly large archive files do not delay MQTT or web startup.
- `set web on|off`
- `set.web on|off`: enables or disables the local HTTPS panel.
- `set web.stats on|off`
- `set.web.stats on|off`: enables or disables the dedicated `/stats` page and historical stats collection.
- `purge sd`: deletes files and directories from the mounted SD card archive, then recreates the empty `/stats` archive directory so runtime stats capture can continue. It does not erase the internal repeater filesystem or stored settings.

### Runtime Diagnostics

- `memory`: shows current heap and PSRAM usage.
- `stats-core`: shows battery, uptime, sticky error count, and outbound queue depth.
- `stats-radio`: shows radio noise floor, last RSSI, last SNR, and TX/RX airtime.
- `stats-packets`: shows packet receive/send totals, flood/direct breakdown, and receive errors.

> If `noise_floor` reports `0`, check `get agc.reset.interval`; if it is not `0`, try `set agc.reset.interval 0` and test again.

### Flood Forwarding Limit

- `get flood.max.unscoped`: shows the hop limit for unscoped flood packets.
- `set flood.max.unscoped <0-64>`: sets the hop limit for unscoped flood packets.
- `get flood.max.advert`: shows the hop limit for flooded advert packets.
- `set flood.max.advert <0-64>`: sets the hop limit for flooded advert packets.

Observer builds default `flood.max.unscoped` to `64`. Lower values can limit how far unscoped flood traffic is repeated while leaving scoped/region flood forwarding controlled by `flood.max`.

### Board Battery Reporting

- On observer builds, background battery sampling used for MQTT/status history is rate-limited to about once per minute. Explicit status and telemetry requests still refresh the reading immediately.

### T-Beam 1W Fan Control

These commands come from upstream MeshCore and are available on `LilyGo_TBeam_1W_*` builds (repeater, room server, and companion CLI).

- `get fan`: shows the fan mode, last NTC board temperature, fan state, and remaining TX cooldown, for example `> auto 34.5C fan=on cd=12s`.
- `set fan auto`: thermal control with hysteresis plus a post-TX cooldown; persists across reboot.
- `set fan on`: forces the fan on and persists that mode across reboot.
- `set fan off`: forces the fan off and persists that mode across reboot.
- `set fan.lo <C>`: auto mode turns the fan off below this temperature (`0`-`100`, must be below `fan.hi`).
- `set fan.hi <C>`: auto mode turns the fan on at or above this temperature (must be above `fan.lo`, max `120`).

Auto mode behaviour:

- turns the fan on during TX and keeps it on for `15s` afterward
- otherwise turns the fan on at `fan.hi` (default `36C`) and off below `fan.lo` (default `30C`)
- keeps the fan on if the NTC reading is unavailable or implausible

Notes:

- default fan mode is `auto`
- the earlier EastMesh `set fan timeout <Ns>` command has been removed; the post-TX cooldown is now fixed
- fan settings saved by older EastMesh builds are not migrated, so the fan returns to `auto` with the default thresholds after upgrading

## Room Server MQTT Bridge Commands

These commands are available on `*_room_server_mqtt_bridge` firmware targets. There is one for every board that has a `*_repeater_observer_mqtt_bridge` build, for example `heltec_v4_room_server_mqtt_bridge`.

This firmware is a normal MeshCore room server with four EastMesh additions:

- Wi-Fi and the local HTTPS web panel
- an MQTT bridge that relays mesh packets through a broker, like the `*_repeater_observer_mqtt_bridge` repeaters
- room sync, which shares room posts with other room servers on the same topic
- repeating, which is on by default so bridged packets reach LoRa

### First-Time Setup

From the serial console:

```text
set wifi.ssid <your-ssid>
set wifi.pwd <your-password>
set bridge.peer.host mqtt.example.com
set bridge.peer.port 1883
set bridge.peer.username <user>
set bridge.peer.password <password>
set bridge.secret <shared-secret>
set bridge.topic EastMesh
get bridge.sync
```

Every room server that should share posts must use the same broker, `bridge.secret`, and `bridge.topic`.

### Wi-Fi And NTP

- `get wifi.status`, `get wifi.ssid`, `get wifi.powersaving`
- `set wifi.ssid <ssid>`, `set wifi.pwd <password>`, `set wifi.powersaving none|min|max`
- `wifi reconnect`
- `get ntp.server1|2|3`, `set ntp.server1|2|3 <host>`

These work the same as on observer builds. Wi-Fi is only brought up while the web panel or the bridge needs it.

### Bridge Broker

- `get bridge.status`: shows bridge state (`online`, `connecting`, `no-wifi`, `off`, or `no-host`), the peer broker, and whether repeat is on.
- `get bridge.enabled`, `set bridge.enabled on|off`
- `get bridge.peer.host`, `set bridge.peer.host <host>`: broker hostname or IP address (no scheme, port, or path).
- `get bridge.peer.port`, `set bridge.peer.port <1-65535>`: default `1883`.
- `get bridge.peer.username`, `set bridge.peer.username <user>`
- `get bridge.peer.password`, `set bridge.peer.password <password>`: `get` only reports `set` or `-`.
- `get bridge.secret`, `set bridge.secret <secret>`: shared secret (up to 15 characters). It encrypts both bridged mesh packets and room sync posts.

The bridge uses plain MQTT over TCP. TLS (`8883`) and WebSockets are not supported on the bridge connection.

Mesh packets are bridged on the shared `meshcore/bridge/packets` topic, so a room server bridges with any EastMesh MQTT bridge repeaters that use the same broker and secret.

### Room Sync

- `get bridge.topic`: shows the room sync topic, or `-` when sync is off.
- `set bridge.topic <name>`: sets the room sync topic and reconnects the bridge. Use 1-32 characters from `A-Z a-z 0-9 - _`.
- `set bridge.topic off`: turns room sync off.
- `get bridge.sync`: shows `sync:<state> topic:<name> tx:<n> rx:<n> dup:<n> bad:<n> drop:<n>`.

How room sync works:

- A post that a client sends to this room, or an admin `room.post`, is published to `meshcore/rooms/<topic>`.
- Every other room server with the same topic and secret stores the post under its original author and pushes it to its own logged-in clients.
- Posts are encrypted and authenticated with a key derived from the topic and `bridge.secret`. A room server with a different secret counts the message under `bad` and ignores it.
- A room server does not re-publish posts it received, so posts do not loop. Duplicate deliveries count under `dup`.
- There is no backfill. A room server that is offline when a post is made does not receive it later.
- Posts older than 24 hours by the sender's clock are dropped as replays. Keep NTP working so clocks stay correct.
- Clients see a synced post under the original author's public key. The app shows the author's name only if it already knows that contact.

`room.post <text>` from the serial console or web panel is a quick way to test sync without a client app.

### Repeating

- `get repeat`, `set repeat on|off`

Upstream MeshCore room servers no longer repeat. This firmware turns repeating back on by default so packets arriving over the bridge are transmitted on LoRa. A board that already has saved settings from an upstream room server keeps its existing `repeat off` until you run `set repeat on`.

### Web Panel

- `get web`, `get web.status`
- `set web on|off`
- `get mqtt.client_env`: shows the PlatformIO env this firmware was built for (same command as observer builds), for example `heltec_v4_room_server_mqtt_bridge`. The panel's firmware upload uses it to refuse `.bin` files named for a different board.

The room server panel is described in [Room Server Web Panel](./web-panel.md#room-server-web-panel).

## Web Panel CLI Access

When the repeater web panel is enabled and you are authenticated, the browser CLI panel can run the same CLI commands accepted by the repeater.

Notes:

- the panel still uses the repeater admin password for access
- commands run with the same care as if you typed them into the repeater CLI directly
- this is intended for local admin use on a trusted network
- `start ota` releases the local HTTP redirect listener on port `80` so the OTA HTTP listener can take over without stopping the rest of the repeater services, regardless of whether the command is run from the web panel, serial CLI, or a remote companion/app CLI session
- if the old redirect listener has not fully released port `80`, the OTA listener retries for up to about 30 seconds before giving up
- the web `Purge SD` button runs `purge sd` after browser confirmation
- `start ota` uses the repeater's existing Wi-Fi address when already connected, or starts the `MeshCore-OTA` access point when Wi-Fi is not connected
- the `/app` Regions shortcut runs the existing MeshCore region commands in sequence: `region put au`, `region put au-STATE`, `region allowf au`, `region allowf au-STATE`, then `region save`

## Heap Health Check Script

`eastmesh-tools/web-heap-check.sh <host> [--stress] [--rounds N]` reads `memory`, `get web.status`, `get mqtt.status` and `get wifi.status` over the HTTPS API and, with `--stress`, simulates browser page-load bursts before re-reading `memory`. Authenticate with `REPEATER_PASSWORD` or `REPEATER_TOKEN` in the environment (or enter the password at the prompt). Watch `heap_max` (largest internal block; TLS needs ~40KB), `heap_min`, and the `heals`/`deferred`/`leaked` counters.
