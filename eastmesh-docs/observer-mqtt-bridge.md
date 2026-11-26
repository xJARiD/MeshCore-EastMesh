# Observer MQTT Bridge

The `observer-eastmesh-bridge-mqtt` track (`*_repeater_observer_mqtt_bridge` builds) links meshes that radio can't reach. Each bridge node relays mesh packets through an **MQTT broker you run**, and the other bridge nodes put them back on their own LoRa.

These builds are full [observers](./observer.md), so everything on that page applies too. Every observer board has a `*_repeater_observer_mqtt_bridge` build. [Room servers](./room-server.md) can join the same bridge.

Every command is listed in the [CLI Reference](./custom-cli.md#mqtt-bridge-settings-for-observer-mqtt-bridge-builds).

## Before You Deploy

Bridges aren't for everyone. Most repeaters should stay plain repeaters or observers, and a bridge should be a deliberate choice.

- **Every bridged packet is retransmitted on the other side.** Adverts, floods and chat from one mesh are repeated over LoRa on the other, so both meshes carry more traffic and use more airtime. Bridging busy meshes can congest both.
- **Talk to the people who run the meshes you're joining.** A bridge links meshes that may have been kept separate on purpose, by region, radio config or community. Don't bridge into a shared mesh without agreement.
- **Treat the broker as part of your network.** The bridge uses plain MQTT over TCP, not TLS. Use a broker with usernames and passwords, and don't run it open to the internet. Mesh messages stay MeshCore-encrypted end to end.
- **Check your radio settings before you connect.** A bridge transmits traffic from elsewhere, so set the correct regional radio preset first. New installs start on the upstream default, which is not the Australian band.

## Uplink Versus Bridge

The bridge is separate from the observer's MQTT uplink, and the two use different brokers:

| | MQTT uplink | MQTT mesh bridge |
| --- | --- | --- |
| Purpose | publish telemetry to CoreScope and other services | carry encrypted mesh packets between radios |
| Broker | `eastmesh-au`, `meshmapper`, custom | your own broker (`bridge.peer.*`) |
| Connection | WSS (TCP for custom) | plain MQTT over TCP only |
| Topic | `meshcore/<IATA>/...` | `meshcore/bridge/packets` (fixed) |

## How It Works

- Every bridge node connects to the same broker and publishes to and subscribes on `meshcore/bridge/packets`.
- Packets are encrypted with `bridge.secret` (up to 15 characters) and checked with a checksum. A node with a different secret can't read them.
- Each node remembers which packets it has seen, so packets don't loop between nodes that hear the same traffic.
- Packets received from the broker are handled after `bridge.delay` (default 500 ms), as if heard over LoRa.
- TLS (`8883`) and WebSockets are not supported on the bridge connection. Run the broker on a network you trust, or tunnel it.

## Setup

### 1. Run A Broker

Any MQTT broker that every bridge node can reach works. This example uses [Mosquitto](https://mosquitto.org/download/) on a LAN PC at `192.168.1.145`.

Add to `mosquitto.conf` (on a Windows service install, `C:\Program Files\Mosquitto\mosquitto.conf`):

```conf
listener 1883 0.0.0.0
allow_anonymous false
password_file C:\Program Files\Mosquitto\passwd
```

For lab testing only, `allow_anonymous true` works without a password file.

Create a user (from an admin prompt):

```cmd
cd "C:\Program Files\Mosquitto"
mosquitto_passwd -c passwd bridgeuser
```

Restart the service. Don't also run `mosquitto -v` while the service owns port 1883.

```powershell
Restart-Service mosquitto
```

Check it listens on the LAN, not just localhost, and open the firewall for port 1883:

```cmd
netstat -ano | findstr ":1883"
```

Expect `0.0.0.0:1883`, not only `127.0.0.1:1883`. Then test from another machine:

```cmd
mosquitto_pub -h 192.168.1.145 -p 1883 -u bridgeuser -P your-password -t test -m hello
mosquitto_sub -h 192.168.1.145 -p 1883 -u bridgeuser -P your-password -t meshcore/bridge/packets -v
```

Once the bridge is running, binary data on `meshcore/bridge/packets` is normal. The payloads are encrypted mesh frames, not text.

### 2. Configure Each Bridge Node

Do the normal [observer setup](./observer.md#setup) first. Then, on every bridge node, from the serial console or the web panel CLI:

```text
set bridge.peer.host 192.168.1.145
set bridge.peer.port 1883
set bridge.peer.username bridgeuser
set bridge.peer.password your-password
set bridge.secret your-shared-bridge-secret
```

`bridge.peer.host` is a hostname or IP address only, with no scheme, port or path. Every node needs the same broker, credentials and `bridge.secret`.

In the [web panel](./web-panel.md#mqtt-settings), **MQTT Settings** has a **Mesh bridge peer** field for the broker `host:port`, username and password. Set `bridge.secret` from the CLI.

### 3. Check It

```text
get bridge.type
get bridge.enabled
get bridge.peer.host
```

`get bridge.type` should report `mqtt`. Then send something from a companion near one node and confirm it arrives near another.

## What Gets Bridged

Two settings decide what flows each way.

**LoRa to broker** follows `bridge.source`:

- `logTx` (default): only packets the node transmits go to the broker. On a repeater with `repeat on` that's everything it repeats, plus its own packets. With `repeat off`, only its own packets go to the broker.
- `rx` (`set bridge.source rx`): every packet the node hears goes to the broker, whether or not it repeats it.

**Broker to LoRa** follows `repeat`:

- `repeat on` (default on repeaters): packets from the broker are repeated on LoRa like any other packet. Hop limits and region rules still apply.
- `repeat off`: packets from the broker are not put on LoRa, unless [gateway mode](#gateway-mode) is on.

## Gateway Mode

A gateway links its LoRa area to the bridge without acting as a general repeater for local traffic:

```text
set repeat off
set bridge.source rx
set bridge.gateway on
```

- `bridge.source rx` sends everything the gateway hears locally to the broker.
- `bridge.gateway on` puts packets from the broker on LoRa even though `repeat` is off.
- Local LoRa traffic is not repeated on LoRa.

Discovery replies still report the node as a repeater with repeating disabled, because they follow `repeat`.

`bridge.gateway` has no effect while `repeat` is `on`: everything is already repeated.

## Room Servers On The Bridge

[Room servers](./room-server.md) (`*_room_server_mqtt_bridge`) use the same `meshcore/bridge/packets` topic. A room server on the same broker with the same `bridge.secret` joins the bridge automatically. Room servers also sync posts with each other on a separate topic.

## Troubleshooting

- **No traffic crosses the bridge:** check `get bridge.type` is `mqtt` and `get bridge.enabled` is `on`. Then confirm every node has the same broker, credentials and `bridge.secret`.
- **The broker rejects the connection:** check the broker listens on `0.0.0.0` and the firewall allows port 1883. Use `mosquitto_sub` from another machine to confirm.
- **Using port 8883 or a `wss://` URL:** not supported. The bridge uses plain TCP.
- **A node with `repeat off` sends nothing to the broker:** expected with the default `bridge.source logTx`. Use `set bridge.source rx`, or set up [gateway mode](#gateway-mode).

## Related Docs

- [Observer](./observer.md)
- [Room Server](./room-server.md)
- [Repeater Web Panel](./web-panel.md)
- [CLI Reference](./custom-cli.md#mqtt-bridge-settings-for-observer-mqtt-bridge-builds)
