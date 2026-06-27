# Build Locally With uv

This repo uses `uv` for Python tooling and runs PlatformIO through `uv run`.

This page is for building from source. If you just want firmware to flash, start with [Download and Flash Releases](./releases.md) instead.

Use this page when you are changing firmware, testing a target before release, or building a local artifact that is not available from GitHub Releases.

## Setup

From the repo root:

```bash
uv sync
```

## Useful Commands

List build targets:

```bash
bash eastmesh-build.sh list
```

Plain PlatformIO build for a single target:

```bash
uv run pio run -e heltec_v4_repeater_observer
uv run pio run -e heltec_v4_companion_radio_wifi
uv run pio run -e heltec_v4_repeater_bridge_espnow
uv run pio run -e heltec_v4_repeater_observer_espnow
uv run pio run -e Xiao_S3_WIO_repeater_observer_mqtt_bridge
```

Flash a target:

```bash
uv run pio run -e heltec_v4_repeater_observer -t upload --upload-port /dev/tty.usbmodemXXXX
```

Serial monitor:

```bash
uv run pio device monitor --port /dev/tty.usbmodemXXXX --baud 115200
```

## Release-Style Local Builds

If you want the same version metadata used by the release workflows, export the version variables first.

Companion Wi-Fi:

```bash
export FIRMWARE_VERSION=v1.14.1
bash eastmesh-build.sh build-firmware heltec_v4_companion_radio_wifi
```

Observer:

```bash
export FIRMWARE_VERSION=v1.15.0
export EASTMESH_VERSION=v2026.5.1
bash eastmesh-build.sh build-firmware heltec_v4_repeater_observer
```

Repeater ESP-NOW bridge:

```bash
export FIRMWARE_VERSION=v1.15.0
bash eastmesh-build.sh build-firmware heltec_v4_repeater_bridge_espnow
```

Observer ESP-NOW:

```bash
export FIRMWARE_VERSION=v1.15.0
export EASTMESH_VERSION=v2026.5.1
bash eastmesh-build.sh build-firmware heltec_v4_repeater_observer_espnow
```

This produces versioned artifacts in `out/`.

Versioning rule:

- `companion-wifi` and `repeater-bridge-espnow` use the upstream MeshCore version as `FIRMWARE_VERSION`
- `observer-eastmesh`, `observer-eastmesh-bridge-espnow`, and `observer-eastmesh-bridge-mqtt` use the upstream MeshCore version as `FIRMWARE_VERSION` plus the EastMesh release version as `EASTMESH_VERSION`

## Supported `repeater_observer` Boards

These are the full PlatformIO env names used for local source builds and release artifact naming.

```text
Generic_E22_sx1262_repeater_observer
Generic_E22_sx1268_repeater_observer
Heltec_E213_repeater_observer
Heltec_E290_repeater_observer
Heltec_T190_repeater_observer
heltec_tracker_v2_repeater_observer
Heltec_v2_repeater_observer
Heltec_v3_repeater_observer
heltec_v4_repeater_observer
heltec_v4_tft_repeater_observer
Heltec_Wireless_Paper_repeater_observer
Heltec_Wireless_Tracker_repeater_observer
Heltec_WSL3_repeater_observer
LilyGo_T3S3_sx1262_repeater_observer
LilyGo_T3S3_sx1276_repeater_observer
LilyGo_TBeam_1W_repeater_observer
LilyGo_TDeck_repeater_observer
LilyGo_Tlora_C6_repeater_observer
M5Stack_Unit_C6L_repeater_observer
Meshadventurer_sx1262_repeater_observer
Meshadventurer_sx1268_repeater_observer
Meshimi_repeater_observer
nibble_screen_connect_repeater_observer
RAK_3112_repeater_observer
Station_G2_logging_repeater_observer
Station_G2_repeater_observer
T_Beam_S3_Supreme_SX1262_repeater_observer
Tbeam_SX1262_repeater_observer
Tbeam_SX1276_repeater_observer
WHY2025_badge_repeater_observer
Xiao_C6_repeater_observer
Xiao_S3_WIO_repeater_observer
```

## Supported Bridge Boards

Bridge targets can be listed from the repo root:

```bash
bash eastmesh-build.sh list | grep '_repeater_bridge_espnow'
bash eastmesh-build.sh list | grep '_repeater_observer_espnow'
```

Common examples:

```text
heltec_v4_repeater_bridge_espnow
heltec_v4_repeater_observer_espnow
Station_G2_repeater_bridge_espnow
Station_G2_repeater_observer_espnow
T_Beam_S3_Supreme_SX1262_repeater_bridge_espnow
T_Beam_S3_Supreme_SX1262_repeater_observer_espnow
```

## Supported `repeater_observer_mqtt_bridge` Boards

At present only the Xiao S3 WIO observer MQTT bridge target is defined:

```text
Xiao_S3_WIO_repeater_observer_mqtt_bridge
```

List all observer MQTT bridge targets:

```bash
bash eastmesh-build.sh list | grep '_repeater_observer_mqtt_bridge'
```

Bridge firmware is for local ESP-NOW bridge use between nearby repeaters. It is not MQTT-over-WAN or VPN bridging.

## Supported `companion_radio_wifi` Boards

These are the full PlatformIO env names used for local source builds and release artifact naming.

```text
heltec_tracker_v2_companion_radio_wifi
Heltec_v2_companion_radio_wifi
Heltec_v3_companion_radio_wifi
heltec_v4_companion_radio_wifi
heltec_v4_tft_companion_radio_wifi
Heltec_WSL3_companion_radio_wifi
LilyGo_TBeam_1W_companion_radio_wifi
LilyGo_TLora_V2_1_1_6_companion_radio_wifi
nibble_screen_connect_companion_radio_wifi
RAK_3112_companion_radio_wifi
Station_G2_companion_radio_wifi
T_Beam_S3_Supreme_SX1262_companion_radio_wifi
ThinkNode_M2_companion_radio_wifi
ThinkNode_M5_companion_radio_wifi
Xiao_S3_WIO_companion_radio_wifi
```

## Companion Wi-Fi CLI

Current companion Wi-Fi builds support persisted Wi-Fi rescue commands:

- open a serial monitor at `115200` baud
- reboot the device
- long-press the user button within the first 8 seconds after boot to enter `CLI Rescue`
- wait for `========= CLI Rescue =========`
- then run the Wi-Fi rescue commands below from the serial monitor

```text
get wifi.status
get wifi.ssid
get wifi.powersaving
set wifi.ssid <ssid>
set wifi.pwd <password>
set wifi.powersaving none|min|max
```
