# CLI Reference

This page lists the EastMesh-specific CLI commands, grouped by build. It doesn't repeat the full upstream MeshCore CLI.

For setup steps, explanations and troubleshooting, start with the guide for your build:

- [Observer](./observer.md): `*_repeater_observer` and `*_repeater_observer_espnow`
- [Observer MQTT Bridge](./observer-mqtt-bridge.md): `*_repeater_observer_mqtt_bridge`
- [Room Server](./room-server.md): `*_room_server_mqtt_bridge`

Commands work from the serial console, a remote admin session, and the web panel CLI. No-argument `get` commands must be entered exactly as shown.

## Observer Commands

These commands are available on `*_repeater_observer` builds, including the ESP-NOW and MQTT bridge variants. See the [Observer guide](./observer.md).

### MQTT Status And Routing

- `get mqtt.status`: Wi-Fi, NTP, IATA, both broker slots (`p:<broker>:<state>`, `s:<broker>:<state>`), custom transport, status publishing, TX publishing, and `leaked:<n>` (should stay `0`). See [Check it's working](./observer.md#check-its-working).
- `get mqtt.statuscfg`: periodic status publishing as `on` or `off`.
- `get mqtt.client_version`: the `client_version` string published over MQTT.
- `get mqtt.client_env`: the PlatformIO env the firmware was built for.
- `get mqtt.iata`, `set mqtt.iata <code>`: the IATA code used in MQTT topics, for example `MEL`. See [Choose your IATA code](./observer.md#choose-your-iata-code).
- `set mqtt.iata UNSET`: mark the IATA code as not configured. No broker connects while it is `UNSET`.

### MQTT Identity

- `get mqtt.owner`, `set mqtt.owner <64-hex-char-public-key>`: owner public key in the JWT metadata. `mqtt.owner <key>` is shorthand.
- `get mqtt.email`, `set mqtt.email <email>`: owner email in the JWT metadata. `mqtt.email <email>` is shorthand.

### MQTT Message Controls

- `get mqtt.packets`, `set mqtt.packets on|off`: publish received packets. Default `on`.
- `get mqtt.tx`, `set mqtt.tx on|off`: also publish transmitted packets. Default `off`.
- `get mqtt.raw`, `set mqtt.raw on|off`: publish raw payloads on a separate `raw` topic. Default `off`.
- `set mqtt.status on|off`: periodic status messages. Default `on`.

### MQTT Endpoints

At most two brokers can be enabled at once. See [MQTT brokers](./observer.md#mqtt-brokers).

- `get mqtt.eastmesh-au`, `set mqtt.eastmesh-au on|off`
- `get mqtt.meshmapper`, `set mqtt.meshmapper on|off`
- `get mqtt.waev`, `set mqtt.waev on|off`
- `get mqtt.letsmesh-eu`, `set mqtt.letsmesh-eu on|off`: retired
- `get mqtt.letsmesh-us`, `set mqtt.letsmesh-us on|off`: retired
- `get mqtt.custom`, `set mqtt.custom on|off`
- `get mqtt.custom.host`, `set mqtt.custom.host <host>`
- `get mqtt.custom.port`, `set mqtt.custom.port <port>`
- `get mqtt.custom.transport`, `set mqtt.custom.transport tcp|wss`: default `tcp`
- `get mqtt.custom.username`, `set mqtt.custom.username <username>`
- `get mqtt.custom.password`, `set mqtt.custom.password <password>`: `get` only reports `set` when a password is configured

Legacy dotted aliases are also accepted: `mqtt.eastmesh.au`, `mqtt.letsmesh.eu`, `mqtt.letsmesh.us`.

Retired LetsMesh selections are cleared once on upgrade.

### Wi-Fi Settings For Observers

- `get wifi.status`: SSID, state, raw status code, IP, channel and signal, plus gateway health (`gw:ok|lost`) and watchdog reconnects (`wd:<n>`). See [Connectivity watchdog](./observer.md#connectivity-watchdog).
- `get wifi.ssid`, `set wifi.ssid <ssid>`
- `set wifi.pwd <password>`
- `get wifi.powersaving`, `set wifi.powersaving none|min|max`
- `wifi reconnect`: drop the association and rejoin after a full channel scan.

### NTP Settings For Observers

- `get ntp.server1|2|3`, `set ntp.server1|2|3 <host>`: setting a server restarts time sync.

Defaults are `au.pool.ntp.org`, `time.google.com` and `time.cloudflare.com`.

### ESP-NOW Bridge Settings For Observer ESP-NOW Builds

Available on `*_repeater_observer_espnow` builds. The channel must match the Wi-Fi channel; see [Observer ESP-NOW builds](./observer.md#observer-esp-now-builds).

- `get bridge.channel`, `set bridge.channel <1-14>`: restarts the bridge.
- `get bridge.secret`, `set bridge.secret <secret>`: restarts the bridge.

### MQTT Bridge Settings For Observer MQTT Bridge Builds

Available on `*_repeater_observer_mqtt_bridge` and `*_room_server_mqtt_bridge` builds. See the [Observer MQTT Bridge guide](./observer-mqtt-bridge.md).

- `get bridge.type`: `mqtt` on these builds.
- `get bridge.enabled`, `set bridge.enabled on|off`: default `on`.
- `get bridge.peer.host`, `set bridge.peer.host <host>`: hostname or IP address only, no scheme, port or path.
- `get bridge.peer.port`, `set bridge.peer.port <1-65535>`: default `1883`.
- `get bridge.peer.username`, `set bridge.peer.username <user>`
- `get bridge.peer.password`, `set bridge.peer.password <password>`: `get` only reports `set` or `-`.
- `get bridge.secret`, `set bridge.secret <secret>`: up to 15 characters, the same on every bridge node.
- `get bridge.source`, `set bridge.source tx|rx`: what goes to the broker. `logTx` (default) sends what the node transmits; `rx` sends everything it hears. See [What gets bridged](./observer-mqtt-bridge.md#what-gets-bridged).
- `get bridge.delay`, `set bridge.delay <0-10000>`: milliseconds before a packet from the broker is handled. Default `500`.
- `get bridge.gateway`, `set bridge.gateway on|off`: with `repeat off`, still put packets from the broker on LoRa. Default `off`; no effect while `repeat` is `on`. See [Gateway mode](./observer-mqtt-bridge.md#gateway-mode).

### Web Panel Controls

- `get web`, `set web on|off` (or `set.web on|off`): the local HTTPS panel.
- `get web.status`: whether the panel is available, plus `heals:<n>` and `deferred:<n>`. See [Web panel and memory](./observer.md#web-panel-and-memory).
- `get web.stats.status`: whether the `/stats` page and history are enabled, whether recent history is active, PSRAM history availability, and whether the SD archive is mounted.
- `set web.stats on|off` (or `set.web.stats on|off`): the `/stats` page and history collection. History covers battery, radio, supported environment sensors, and satellites on GPS boards. If the SD archive drops while stats are on, the repeater retries the mount periodically.
- `purge sd`: delete everything on the mounted SD archive, then recreate an empty `/stats` directory. It doesn't touch the internal filesystem or settings.

### Runtime Diagnostics

- `memory`: heap and PSRAM use.
- `stats-core`: battery, uptime, sticky error count, and outbound queue depth.
- `stats-radio`: noise floor, last RSSI and SNR, and TX/RX airtime.
- `stats-packets`: packet totals, flood/direct breakdown, and receive errors.

If `noise_floor` reports `0`, check `get agc.reset.interval`. If it isn't `0`, try `set agc.reset.interval 0`.

### Flood Forwarding Limit

- `get flood.max.unscoped`, `set flood.max.unscoped <0-64>`: hop limit for unscoped floods. Observer builds default to `64`.
- `get flood.max.advert`, `set flood.max.advert <0-64>`: hop limit for flooded adverts.

Scoped (region) floods still follow `flood.max`.

### T-Beam 1W Fan Control

These commands come from upstream MeshCore and are available on `LilyGo_TBeam_1W_*` builds (repeater, room server and companion).

- `get fan`: mode, NTC board temperature, fan state and remaining TX cooldown, for example `> auto 34.5C fan=on cd=12s`.
- `set fan auto`: thermal control with hysteresis and a post-TX cooldown. This is the default.
- `set fan on`, `set fan off`: force the fan on or off.
- `set fan.lo <C>`: in auto mode, turn the fan off below this temperature (`0`-`100`, below `fan.hi`). Default `30`.
- `set fan.hi <C>`: in auto mode, turn the fan on at or above this temperature (above `fan.lo`, max `120`). Default `36`.

All fan settings persist across reboots. In auto mode the fan also runs during TX and for 15 seconds afterwards, and stays on if the temperature reading is unavailable.

The earlier EastMesh `set fan timeout <Ns>` command has been removed. Fan settings from older EastMesh builds aren't migrated, so the fan returns to `auto` with default thresholds after upgrading.

## Room Server MQTT Bridge Commands

These commands are available on `*_room_server_mqtt_bridge` builds. See the [Room Server guide](./room-server.md).

### Wi-Fi And NTP

The same as on observers: `get wifi.status`, `get wifi.ssid`, `set wifi.ssid`, `set wifi.pwd`, `get|set wifi.powersaving`, `wifi reconnect`, and `get|set ntp.server1|2|3`. See [Wi-Fi Settings For Observers](#wi-fi-settings-for-observers).

Wi-Fi is only brought up while the web panel or the bridge needs it.

### Bridge Broker

- `get bridge.status`: bridge state (`online`, `connecting`, `no-wifi`, `no-host` or `off`), the broker, and whether repeat is on.
- The `bridge.*` broker commands are the same as on [observer MQTT bridge builds](#mqtt-bridge-settings-for-observer-mqtt-bridge-builds). On room servers, `bridge.secret` encrypts both bridged packets and room sync posts.

### Room Sync

- `get bridge.topic`: the room name used for sync, or `-` when sync is off.
- `set bridge.topic <name>`: set the room name and reconnect. 1 to 32 characters from `A-Z a-z 0-9 - _`.
- `set bridge.topic off`: turn room sync off.
- `get bridge.sync`: `sync:<state> topic:<name> tx:<n> rx:<n> dup:<n> bad:<n> drop:<n>`. See [Troubleshooting](./room-server.md#troubleshooting).
- `room.post <text>`: post to this room as admin. It syncs to every room server on the same room name.

### Repeating

- `get repeat`, `set repeat on|off`: default `on` on this firmware.
- `get bridge.gateway`, `set bridge.gateway on|off`: with `repeat off`, still put packets from the broker on LoRa.

See [Repeating](./room-server.md#repeating) for what each combination does, including [MQTT-connected room only](./room-server.md#mqtt-connected-room-only).

### Web Panel

- `get web`, `get web.status`, `set web on|off`
- `get mqtt.client_env`: the PlatformIO env the firmware was built for, for example `heltec_v4_room_server_mqtt_bridge`. The panel's firmware upload uses it to refuse `.bin` files for a different board.

See [Room Server web panel](./room-server.md#web-panel).

## Web Panel CLI Access

When the web panel is enabled and you're logged in, its CLI box runs the same commands as the serial console.

- Access uses the admin password.
- Treat commands with the same care as on the serial console. The panel is for local admin use on a trusted network.
- `start ota` releases the HTTP redirect listener on port `80` so the OTA listener can take over, wherever the command is run from. If port `80` isn't released yet, the OTA listener retries for about 30 seconds.
- `start ota` uses the existing Wi-Fi address when connected, or starts the `MeshCore-OTA` access point when not.
- The observer panel's `Purge SD` button runs `purge sd` after confirmation.
- The observer panel's Regions shortcut runs `region put au`, `region put au-STATE`, `region allowf au`, `region allowf au-STATE`, then `region save`.

## Heap Health Check Script

`eastmesh-tools/web-heap-check.sh <host> [--stress] [--rounds N]` reads `memory`, `get web.status`, `get mqtt.status` and `get wifi.status` over the HTTPS API. With `--stress`, it simulates browser page-load bursts and reads `memory` again.

Authenticate with `REPEATER_PASSWORD` or `REPEATER_TOKEN` in the environment, or enter the password at the prompt. Watch `heap_max` (the largest internal block; TLS needs about 40 KB), `heap_min`, and the `heals`, `deferred` and `leaked` counters.
