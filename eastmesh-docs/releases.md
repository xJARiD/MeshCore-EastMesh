# Download And Flash Releases

EastMesh release assets are published on:

- <https://github.com/xJARiD/MeshCore-EastMesh/releases>

## Start Here

If this is your first time flashing EastMesh firmware, the easiest path is:

1. Open <https://flasher.eastmesh.au/>.
2. Select the firmware that matches what the device will do.
3. Select the board that matches your exact hardware.
4. Select the version. The flasher defaults to the latest available release.
5. Select the image type:
   - `Update` for a normal firmware update
   - `Full Flash` for a clean full-image flash
6. Click `Flash Firmware`.
7. After flashing, finish setup with `Open MeshCore Config Panel` or the `Serial Console`.

If you are not sure which track you need, start with `companion-wifi` for app-connected companion devices or `observer-eastmesh` for a fixed repeater that should publish to MQTT.

## Pick Your Track

EastMesh publishes five release tracks:

| Track | Use it when | Firmware filename suffix |
| ----- | ----------- | ------------------------ |
| `companion-wifi` | You want a companion device that connects over Wi-Fi instead of BLE or USB. | `*_companion_radio_wifi` |
| `observer-eastmesh` | You want a repeater with Wi-Fi and MQTT uplink, usually feeding broker visibility such as EastMesh/CoreScope. | `*_repeater_observer` |
| `repeater-bridge-espnow` | You want a local ESP-NOW bridge between nearby repeaters, without MQTT uplink or the EastMesh web panel. | `*_repeater_bridge_espnow` |
| `observer-eastmesh-bridge-espnow` | You want one repeater to provide both MQTT uplink and local ESP-NOW bridge duties. | `*_repeater_observer_espnow` |
| `observer-eastmesh-bridge-mqtt` | You want one repeater to provide both MQTT uplink and bidirectional MQTT mesh bridging to a peer broker. | `*_repeater_observer_mqtt_bridge` |

!!! note "Bridge firmware is not a WAN bridge"

    ESP-NOW bridge tracks are for bridging two nearby repeaters that operate on different MeshCore radio configs, for example `Australia (Narrow)` and `Australia (Mid)`.

    The MQTT bridge track forwards mesh packets through a shared topic at a peer MQTT broker you configure. It is separate from MQTT uplink publishing to EastMesh or MeshMapper.

    Bridge tracks do not use MQTT uplink brokers to tunnel mesh traffic over the internet, WAN links, or VPNs.

## Pick The Right Asset

Download the asset that matches your board and firmware type.

Examples:

- `heltec_v4_companion_radio_wifi-v1.14.1-abcdef.bin`
- `heltec_v4_repeater_observer-v1.15.0-eastmesh-v2026.5.1-abcdef.bin`
- `heltec_v4_repeater_observer-v1.15.0-eastmesh-v2026.5.1-abcdef-merged.bin`
- `heltec_v4_repeater_bridge_espnow-v1.15.0-abcdef.bin`
- `heltec_v4_repeater_observer_espnow-v1.15.0-eastmesh-v2026.5.1-abcdef.bin`
- `Xiao_S3_WIO_repeater_observer_mqtt_bridge-v1.15.0-eastmesh-v2026.7.0-abcdef.bin`

The important part is the board/env prefix:

- `*_companion_radio_wifi`
- `*_repeater_observer`
- `*_repeater_bridge_espnow`
- `*_repeater_observer_espnow`
- `*_repeater_observer_mqtt_bridge`

## Which File To Flash

If you are using <https://flasher.eastmesh.au/>, choose the image type in the flasher:

- `Update` = normal firmware update
- `Full Flash` = clean full-image flash

If you are manually downloading files from GitHub Releases, use the filename instead:

Use the standard `.bin` file when you are updating an existing device with the same target and partition layout.

Use the `-merged.bin` file when you want a clean install after erasing flash. This is the full ESP32 image and is intended to be flashed from address `0x0`.

Practical rule:

- `.bin` = incremental update
- `-merged.bin` = erase and clean flash

## Flashing Flow

1. Select firmware.
2. Select board.
3. Select version. The latest version is selected by default.
4. Select image type: `Update` or `Full Flash`.
5. Click `Flash Firmware`.
6. After flashing, use `Open MeshCore Config Panel` or the `Serial Console` for post-flash setup such as Wi-Fi, MQTT, bridge, or radio settings.

## Recommended Flasher

The recommended flasher is:

- <https://flasher.eastmesh.au/>

It includes native support for the common EastMesh firmware types and can also flash custom firmware files:

- `companion_radio_wifi` firmware
- `repeater_observer` firmware
- custom firmware files

For bridge releases, use the matching bridge option if the flasher shows one. Otherwise, download the release asset yourself and flash it as a custom firmware file.

Recommended usage:

- use `Update` when you are updating an existing device
- use `Full Flash` when you want a clean full-image flash

After flashing, there are two useful setup paths in the flasher.

### Open MeshCore Config Panel

Use `Open MeshCore Config Panel` to access the guided MeshCore setup tools:

- `Repeater Setup`
- `Console`

#### Repeater Setup

`Repeater Setup` is the guided first-time repeater flow.

It is the traditional way to configure a repeater after flashing, including:

- device name
- latitude and longitude
- admin and guest passwords
- radio settings, including preset selection
- advert interval
- flood advert interval
- flood max
- some advanced repeater settings

As of `v1.2.1`, the local repeater web panel also includes the same common repeater settings, so users can complete initial setup there and return for occasional troubleshooting or configuration changes. On observers that need maximum headroom, it is still best to disable the panel again when you are finished.

#### Console

`Console` is the raw CLI interface.

It is especially useful, and often required, for initial Wi-Fi setup on Wi-Fi-capable firmware tracks:

- `set wifi.ssid <your-ssid>`
- `set wifi.pwd <your-password>`

This applies to:

- `companion_radio_wifi`
- `repeater_observer`
- `repeater_observer_espnow`

### Serial Console

Use `Serial Console` when you want to connect over USB and type CLI commands directly.

The flasher can connect and disconnect from the device over USB. It also includes preset CLI commands that populate the command input for easier setup:

- `set wifi.ssid`
- `set wifi.pwd`
- `get wifi.status`
- `get mqtt.status`
- `set web on`
- `get web.status`

For `set` commands, complete the command in the input box before sending it. For example, select `set wifi.ssid`, add your Wi-Fi network name, then press `Send`.

## Common First Steps

### Observer

`repeater_observer` builds include the EastMesh MQTT additions. Depending on the board, they may also include the local web panel.

Typical first steps after flashing:

- set `wifi.ssid`
- set `wifi.pwd`
- set `mqtt.iata`
- confirm `get mqtt.status`
- optionally set `mqtt.owner` and `mqtt.email`
- optionally enable a second broker such as `meshmapper`

### Observer ESP-NOW

`repeater_observer_espnow` builds combine the observer role with local ESP-NOW bridge support.

Typical first steps after flashing:

- set `wifi.ssid`
- set `wifi.pwd`
- set `mqtt.iata`
- confirm `get mqtt.status`
- check `get wifi.status` and note the connected Wi-Fi channel
- set `bridge.channel` to match that Wi-Fi channel
- set the same `bridge.secret` on every local ESP-NOW bridge node that should talk together

### Observer MQTT Bridge

`repeater_observer_mqtt_bridge` builds combine the observer role with bidirectional MQTT mesh bridging.

Typical first steps after flashing:

- set `wifi.ssid`
- set `wifi.pwd`
- set `mqtt.iata`
- confirm `get mqtt.status`
- set `bridge.peer.host` and `bridge.peer.port` to your peer MQTT broker
- set the same `bridge.secret` on every MQTT bridge node that should talk together

### Repeater ESP-NOW Bridge

`repeater_bridge_espnow` builds are for local ESP-NOW bridge nodes without MQTT uplink.

Typical first steps after flashing:

- configure the normal repeater settings for the board
- set the intended MeshCore radio preset/config
- set the same bridge channel and secret on the local bridge pair
- confirm both bridge repeaters are physically nearby enough for ESP-NOW to work

### Companion Wi-Fi

`companion_radio_wifi` builds are for companion devices that expose the app interface over Wi-Fi instead of BLE or USB.

Typical first steps after flashing:

- set Wi-Fi credentials
- confirm `get wifi.status`
- connect your client app to the companion over Wi-Fi
