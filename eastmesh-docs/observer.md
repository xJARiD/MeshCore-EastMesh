# Observer

An observer is a normal MeshCore repeater that also connects to Wi-Fi and publishes what it hears to MQTT brokers such as EastMesh Core. That data feeds [CoreScope](https://core.eastmesh.au), the EastMesh network map and telemetry view.

**Observer builds are intended for Eastern Australia repeaters only.** Running one elsewhere can skew shared network data.

| Release track | Build suffix | What it adds |
| --- | --- | --- |
| `observer-eastmesh` | `*_repeater_observer` | Wi-Fi, MQTT uplink, and the local web panel on supported ESP32 boards |
| `observer-eastmesh-bridge-espnow` | `*_repeater_observer_espnow` | everything above, plus a local [ESP-NOW bridge](#observer-esp-now-builds) |
| `observer-eastmesh-bridge-mqtt` | `*_repeater_observer_mqtt_bridge` | everything above, plus an [MQTT mesh bridge](./observer-mqtt-bridge.md) through a broker you run |

Everything on this page applies to all three. Every command is listed in the [CLI Reference](./custom-cli.md#observer-commands).

## Setup

After [flashing](./releases.md), connect to the serial console or the [web panel](./web-panel.md) and run:

```text
set wifi.ssid <your-ssid>
set wifi.pwd <your-password>
get wifi.status
set mqtt.iata <code>
get mqtt.status
```

Then, optionally:

- `set mqtt.owner <your-companion-public-key>` and `set mqtt.email <email>`, so the broker can associate the observer with you
- enable a second broker, for example `set mqtt.meshmapper on`

On boards with the web panel, `set web on` gives you a browser UI for the same settings. When setup is done, `set web off` leaves the most memory for MQTT. See [Web panel and memory](#web-panel-and-memory).

## Choose Your IATA Code

The IATA code is the three-letter airport code for your area, for example `BNE` or `MEL`. It goes into every MQTT topic the observer publishes (`meshcore/<IATA>/<device>/...`), and CoreScope uses it to group observers by area. Pick the nearest airport.

- New installs start as `UNSET`. **While the code is `UNSET`, no broker connects**, so this is the one setting you can't skip.
- The web panel offers a list of Eastern Australian codes. If yours is missing, `set mqtt.iata <code>` from the CLI accepts any code.
- Changing the code publishes a retained `offline` status on the old topic, then reconnects under the new one.
- `set mqtt.iata UNSET` takes the observer off MQTT until a real code is saved.

## MQTT Brokers

An observer can publish to **at most two brokers at once**.

| Broker | Command | Connection |
| --- | --- | --- |
| EastMesh Core | `set mqtt.eastmesh-au on` | WSS, verified TLS, MeshCore JWT auth |
| MeshMapper (global) | `set mqtt.meshmapper on` | `wss://mqtt.meshmapper.net:443/mqtt`, verified TLS, JWT auth |
| WAEV | `set mqtt.waev on` | `wss://mqtt.waev.app:443/mqtt`, verified TLS, JWT auth with a 1-hour token lifetime (the firmware handles this) |
| Custom | `set mqtt.custom on` | your own broker, TCP or WSS, username and password |
| LetsMesh EU | `set mqtt.letsmesh-eu on` | `wss://mqtt-eu-v1.letsmesh.net:443/mqtt`, verified TLS, JWT auth |
| LetsMesh US | `set mqtt.letsmesh-us on` | `wss://mqtt-us-v1.letsmesh.net:443/mqtt`, verified TLS, JWT auth |

The curated brokers use JWT tokens signed with the observer's own identity, with the broker host as the token audience, so they need no passwords.

### Custom Broker

```text
set mqtt.custom.host mqtt.example.com
set mqtt.custom.port 1883
set mqtt.custom.username <user>
set mqtt.custom.password <password>
set mqtt.custom on
```

- The default transport is plain TCP. `set mqtt.custom.transport wss` switches to MQTT over secure WebSockets on the fixed `/mqtt` path, verified against the ESP-IDF root CA bundle.
- Custom brokers use username and password, not JWT.
- Topics are the same `meshcore/<IATA>/<device>/<leaf>` layout as the curated brokers.

### What Gets Published

| Setting | Default | Publishes |
| --- | --- | --- |
| `mqtt.packets` | on | packets the observer hears |
| `mqtt.tx` | off | packets the observer transmits |
| `mqtt.raw` | off | raw packet payloads on a separate `raw` topic |
| `mqtt.status` | on | periodic status messages |

Turning a connected broker off publishes a retained `"status":"offline"` message before disconnecting, so CoreScope shows the observer as offline rather than stale.

## Check It's Working

`get mqtt.status` shows Wi-Fi, NTP, the IATA code, and both broker slots:

- `p:<broker>:<state>` is the primary slot and `s:<broker>:<state>` the secondary, or `-` when empty. A custom broker appears as `custom:<transport>:<state>`, for example `p:custom:wss:up`.
- `up` means connected and `conn` means connecting.
- `leaked:<n>` counts MQTT clients deliberately left unfreed after a heap problem at teardown. It should stay `0`.

`get wifi.status` shows the SSID, state, IP, channel and signal. It also shows:

- `gw:ok` or `gw:lost`: whether the gateway is answering
- `wd:<n>`: how many times the connectivity watchdog has forced a reconnect since boot

### Connectivity Watchdog

Some access points keep advertising Wi-Fi after losing their internet uplink, so the observer looks connected but nothing gets through. While Wi-Fi reports connected, the observer probes its gateway every 30 seconds. If the gateway stays silent for 3 minutes, the observer forces a full reconnect, backing off up to 48 minutes between attempts while the outage lasts.

To force a reconnect yourself, run `wifi reconnect`. It drops the association and rejoins after a full channel scan.

### Time

MQTT authentication needs the correct time, so observers sync over NTP. The defaults are `au.pool.ntp.org`, `time.google.com` and `time.cloudflare.com`. Change them with `set ntp.server1|2|3 <host>` if your network blocks those.

## Web Panel And Memory

On supported ESP32 boards the observer can serve a local HTTPS panel for setup, settings and stats. See [Repeater Web Panel](./web-panel.md).

The panel's TLS sessions use the same internal memory as the MQTT connections, so:

- use `set web on` for setup and troubleshooting
- use `set web off` afterwards on observers that need maximum headroom, especially with two WSS brokers

`get web.status` reports `heals:<n>` (panel restarts because memory was too fragmented for a TLS handshake) and `deferred:<n>` (start attempts postponed until memory recovered). For a scripted check, `eastmesh-tools/web-heap-check.sh` is described in the [CLI Reference](./custom-cli.md#heap-health-check-script).

## Observer ESP-NOW Builds

`*_repeater_observer_espnow` builds add a local ESP-NOW bridge between nearby repeaters, for example to link a repeater on `Australia (Narrow)` with one on `Australia (Mid)`. ESP-NOW is a short-range Wi-Fi radio link, not an internet bridge. To bridge distant meshes over the internet, use the [Observer MQTT Bridge](./observer-mqtt-bridge.md).

Because ESP-NOW shares the Wi-Fi radio, **the bridge channel must match the Wi-Fi channel**:

1. Run `get wifi.status` and note `channel:<n>`.
2. Run `get bridge.channel`.
3. If they differ, run `set bridge.channel <n>`.
4. Use the same `bridge.channel` and `bridge.secret` on every ESP-NOW node that should talk together.

```text
> get wifi.status
> ssid:EastMesh-IoT status:connected code:3 state:connected ip:192.168.1.50 channel:6 rssi:-61 quality:78% signal:good gw:ok wd:0
> get bridge.channel
> 1
> set bridge.channel 6
OK
```

After `set bridge.channel`, the bridge and web panel drop briefly while the radio restarts. It can look like a reboot.

## Tuning

- **Flood hop limit:** observer builds default `flood.max.unscoped` to `64`. Lower it to limit how far unscoped flood traffic is repeated. Scoped (region) floods still follow `flood.max`.
- **Battery sampling:** background battery readings for MQTT and stats history are taken about once a minute. Status and telemetry requests still read it immediately.

## Troubleshooting

- **No broker connects:** check `get mqtt.iata` isn't `UNSET`, then `get wifi.status` and the NTP part of `get mqtt.status`.
- **Connected to Wi-Fi but unreachable:** check `gw:` in `get wifi.status`, or run `wifi reconnect`.
- **MQTT drops while the web panel is open:** run `set web off` when you're finished. See [Web panel and memory](#web-panel-and-memory).
- **`noise_floor` reports `0`:** check `get agc.reset.interval`. If it isn't `0`, try `set agc.reset.interval 0`.
- **Memory questions:** `memory` shows heap and PSRAM use. `stats-core`, `stats-radio` and `stats-packets` show runtime counters.

## Related Docs

- [Observer MQTT Bridge](./observer-mqtt-bridge.md)
- [Repeater Web Panel](./web-panel.md)
- [Repeater Web API](./api.md)
- [Migration From xJARiD/MeshCore](./migration.md)
- [CLI Reference](./custom-cli.md#observer-commands)
