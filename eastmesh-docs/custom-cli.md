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

- `get mqtt.status`: shows Wi-Fi, NTP, IATA, endpoint status, status publishing state, and TX state.
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
- `get mqtt.letsmesh-eu`
- `set mqtt.letsmesh-eu on|off`
- `get mqtt.letsmesh-us`
- `set mqtt.letsmesh-us on|off`
- `get mqtt.custom`
- `set mqtt.custom on|off`
- `get mqtt.custom.host`
- `set mqtt.custom.host <host>`
- `get mqtt.custom.port`
- `set mqtt.custom.port <port>`
- `get mqtt.custom.username`
- `set mqtt.custom.username <username>`
- `get mqtt.custom.password`: shows `set` when a custom password is configured.
- `set mqtt.custom.password <password>`

Notes:

- new observer installs default `mqtt.iata` to `UNSET`
- `letsmesh-eu` and `letsmesh-us` remain off by default unless already configured in saved prefs
- if `mqtt.iata` is `UNSET`, enabled MQTT brokers will not connect
- custom MQTT uses normal MQTT over TCP with the configured username and password, not JWT authentication
- custom MQTT uses the same `meshcore/<IATA>/<device>/<leaf>` topics as the curated brokers
- turning off a connected broker publishes a retained MQTT status update with `"status":"offline"` before the client disconnects
- changing `mqtt.iata` away from a configured value also publishes retained offline status to the old status topic, restarts connected broker clients, and reconnects under the new topic path

Legacy dotted aliases are also accepted:

- `mqtt.eastmesh.au`
- `mqtt.letsmesh.eu`
- `mqtt.letsmesh.us`

### Wi-Fi Settings For Observers

- `get wifi.status`: shows SSID, connection state, raw Wi-Fi status code, IP, channel, and signal when connected.
- `get wifi.ssid`: shows the configured Wi-Fi SSID.
- `set wifi.ssid <ssid>`: sets the Wi-Fi SSID.
- `set wifi.pwd <password>`: sets the Wi-Fi password.
- `get wifi.powersaving`: shows the current Wi-Fi power save mode.
- `set wifi.powersaving none|min|max`: sets Wi-Fi power saving mode.

### ESP-NOW Bridge Settings For Observer ESP-NOW Builds

These commands are available on `*_repeater_observer_espnow` firmware targets.

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
> ssid:EastMesh-IoT status:connected code:3 state:connected ip:192.168.1.50 channel:6 rssi:-61 quality:78% signal:good
> get bridge.channel
> 1
> set bridge.channel 6
OK
```

### Web Panel Controls

- `get web`
- `get web.status`: shows whether the local HTTPS panel is available.
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

These commands are only available on `LilyGo_TBeam_1W_*` repeater builds.

- `get fan`: shows the current fan mode, current fan state, and the last NTC-based board temperature when available.
- `set fan auto`: returns the fan to automatic control and persists that mode across reboot.
- `set fan on`: forces the fan on and persists that mode across reboot.
- `set fan off`: forces the fan off and persists that mode across reboot.
- `set fan timeout <Ns>`: changes the automatic post-TX hold window in seconds and persists it across reboot, for example `set fan timeout 45s`.

Auto mode behaviour:

- forces the fan on during TX and keeps it on for the configured timeout afterward
- otherwise turns the fan on at `48C`
- turns it back off at `42C`
- keeps the fan on if the NTC reading is unavailable

Notes:

- default repeater fan mode is `auto`
- default post-TX timeout is `30s`
- fan mode and timeout are stored in repeater prefs and survive reboot
- only `LilyGo_TBeam_1W_*` repeater builds use these persisted fan settings
- accepted range is `0s` to `600s`

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

## Companion Wi-Fi Rescue Commands

These commands are available in the serial rescue CLI for `*_companion_radio_wifi` builds.

To enter `CLI Rescue`:

- open a serial monitor at `115200` baud
- reboot the device
- long-press the user button within the first 8 seconds after boot
- wait for `========= CLI Rescue =========`

- `get wifi.status`: shows configured SSID, connection status, raw Wi-Fi status code, IP, channel, and signal when connected.
- `get wifi.ssid`: shows the configured Wi-Fi SSID.
- `get wifi.powersaving`: shows the current Wi-Fi power saving mode.
- `set wifi.ssid <ssid>`: saves a Wi-Fi SSID and immediately retries connection.
- `set wifi.pwd <password>`: saves a Wi-Fi password and immediately retries connection.
- `set wifi.powersaving none|min|max`: changes the Wi-Fi power save mode.

Companion Wi-Fi builds also still support the existing rescue commands such as:

- `set pin <6-digit-pin>`
- `rebuild`
- `erase`
- `ls ...`
- `cat ...`
- `rm ...`
- `reboot`
