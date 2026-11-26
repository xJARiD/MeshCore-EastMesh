# Room Server

The `room-server-eastmesh-bridge-mqtt` track (`*_room_server_mqtt_bridge` builds) is a normal MeshCore room server with four EastMesh additions:

- **Wi-Fi and a web panel** for setup in a browser
- **Room sync:** posts are shared with every other room server on the same broker and room name, so one room can span several areas
- **An MQTT bridge** that can relay mesh packets like the [Observer MQTT Bridge](./observer-mqtt-bridge.md)
- **Repeating**, with a choice of repeating everything, only bridged packets, or nothing

There is a room server build for every board with a `*_repeater_observer_mqtt_bridge` build, for example `heltec_v4_room_server_mqtt_bridge`.

Every command is listed in the [CLI Reference](./custom-cli.md#room-server-mqtt-bridge-commands).

## Setup

You need an MQTT broker that every room server can reach. Any broker works. [Run a broker](./observer-mqtt-bridge.md#1-run-a-broker) shows a Mosquitto example.

1. [Flash](./releases.md) the room server and connect to the serial console.
2. Join Wi-Fi:

    ```text
    set wifi.ssid <your-ssid>
    set wifi.pwd <your-password>
    ```

3. Open the [web panel](#web-panel) at the IP address shown on the display or in `get wifi.status`.
4. Pick the radio preset and the AU region, then reboot.
5. Set the broker, secret and room name, in the panel or the CLI:

    ```text
    set bridge.peer.host mqtt.example.com
    set bridge.peer.port 1883
    set bridge.peer.username <user>
    set bridge.peer.password <password>
    set bridge.secret <shared-secret>
    set bridge.topic EastMesh
    ```

6. Check that `get bridge.status` and `get bridge.sync` both report `online`.

Every room server that should share posts needs the **same broker, `bridge.secret` and room name** (`bridge.topic`).

## Room Sync

The room name (`bridge.topic`) decides which room servers share posts. Use 1 to 32 characters from `A-Z a-z 0-9 - _`. `set bridge.topic off` turns sync off.

**Room sync shares everything posted to the room.** Anyone running a room server with the same broker, room name and secret can read and post to it, so choose who you share those with.

- A post a client makes to this room, or an admin `room.post`, is published to `meshcore/rooms/<room name>`.
- Every other room server with the same room name and secret stores the post under its original author and pushes it to its own logged-in clients.
- Posts are encrypted and authenticated with a key derived from the room name and `bridge.secret`. A room server with a different secret ignores them.
- Posts received from another room server are not published again, so they don't loop.
- **There is no backfill.** A room server that is offline when a post is made never receives it.
- Posts more than 24 hours old by the sender's clock are dropped as replays, so keep NTP working.
- Clients see a synced post under the original author's public key. The app shows the author's name only if it already knows that contact.

To test sync without a client app, run `room.post hello` from the serial console or the web panel CLI, then check `get bridge.sync` on another room server.

## Repeating

Repeating controls whether the room server passes on **other nodes'** packets. Whatever you choose, the room always handles its own logins and posts over LoRa and the bridge, and room sync keeps working.

The web panel's **Repeating** control sets two commands:

| Panel option | `repeat` | `bridge.gateway` | Other nodes' LoRa traffic to the broker | Broker traffic onto LoRa |
| --- | --- | --- | --- | --- |
| Repeat all packets | `on` | `off` | yes | yes |
| Repeat bridged packets only | `off` | `on` | no | yes, all of it |
| Room traffic only | `off` | `off` | no | only packets addressed to the room |

This assumes the default `bridge.source logTx`, where the room sends the broker what it transmits on LoRa. See [What gets bridged](./observer-mqtt-bridge.md#what-gets-bridged).

Upstream MeshCore room servers don't repeat. This firmware turns repeating on by default so bridged packets reach LoRa. A board that already has saved settings from an upstream room server keeps `repeat off` until you change it.

### MQTT-Connected Room Only

To run a room that syncs posts and accepts logins over MQTT, without bridging the rest of your mesh, choose **Room traffic only** in the web panel, or:

```text
set repeat off
set bridge.gateway off
```

- Local LoRa traffic, such as companion adverts, stays local.
- Traffic from other meshes on the broker is not put on your LoRa.
- Users can still log in and post on LoRa or over the bridge.
- Posts still sync with every room server on the same broker, room name and `bridge.secret`.

### LoRa Only

To run the room on LoRa with no MQTT at all, use `set bridge.enabled off`. This also stops room sync, because sync uses the bridge's MQTT connection.

## Logins And Access

A room server has two passwords:

- **Room password** (`set guest.password <password>`): what users log in with to read and post. Our builds default it to `hello`, so change it unless the room is meant to be open.
- **Admin password** (`password <new-password>`): full admin access, including the CLI.

**Allow Read-Only Logins** (`set allow.read.only on|off`, default `off`) decides what happens to someone with the wrong or a blank password:

| Password sent | Read-only `off` | Read-only `on` |
| --- | --- | --- |
| Admin password | admin | admin |
| Room password | read and post | read and post |
| Wrong or blank | ignored; the app times out | logged in as a read-only guest |

Read-only guests receive the room's posts, but anything they post is dropped and they only get basic telemetry. Turning read-only on makes the room readable by anyone who can reach it.

### Removing Existing Users

Someone who has logged in since the last restart can log in again with a blank password and keeps their previous access, even after you change the room password or turn read-only logins off. To remove them:

- **Restart the room server.** Only admins are saved across restarts, so everyone else has to log in again with the current password.
- Or remove one user with `setperm <public-key> 0`.

## Web Panel

Open `https://<room-ip>/`, accept the certificate warning, and log in with the room server admin password. The IP address is on the home screen of boards with a display (`IP: -` until Wi-Fi connects), or in `get wifi.status`.

The browser tab and the page heading show the room's name, so several room servers are easy to tell apart.

The panel stays hidden until your session is confirmed. With no session, or after the room server restarts, it goes straight to login.

The panel has:

- **Status line:** under the action buttons, the firmware version from `ver` and the EastMesh build version from `get mqtt.client_version` once settings have loaded.
- **Actions:** `Advert`, `Start OTA`, `Refresh`, `Reboot`, the theme toggle, and `Logout`. `Start OTA` runs `start ota` and after 5 seconds opens the returned `http://<ip>/update` page for a firmware upload.
- **Status tiles:** Wi-Fi, MQTT Bridge and Room Sync, refreshed every 30 seconds while the tab is visible.
- **Room Settings:** room name, latitude and longitude, public key, clock with a `Sync from browser` button, room password (the guest password), read-only logins, and the admin password. See [Logins and access](#logins-and-access).
- **Wi-Fi:** SSID and password. Saving drops the page until the room server rejoins the network, possibly with a new IP address.
- **MQTT Bridge & Room Sync:** bridge on/off, broker `host:port`, username, password, room name, and [Repeating](#repeating). The bridge secret isn't shown; change it with `set bridge.secret <secret>` in the CLI.
- **Radio:** current radio, community radio presets (the same list as the observer panel), the `Region (Australia)` state selector, and TX power. Preset changes apply after a reboot. The region selector runs `region def au <state>` then `region save`.
- **Firmware:** choose the non-merged `.bin` for this board and press `Upload`. The panel refuses release files named for a different board (`<env>-<version>.bin`) and `-merged.bin` USB images. A locally built `firmware.bin` can't be matched by name, so it's allowed with a reminder. The upload is checked, shows progress, and the page reloads after the room server reboots. OTA mode isn't needed.
- **CLI:** quick buttons for `wifi.status`, `bridge.status`, `bridge.sync`, `web.status` and `ver`, plus a free-form command box.

Password fields are write-only and clear after saving. Every control runs a CLI command.

The room server panel has no `/stats` page, firmware update banner, or MQTT observer settings.

## Web API

Room servers serve the same [Web API](./api.md#room-server-builds) as observers, logged in with the room server admin password. Stats history isn't available. Useful commands through `/api/command` include `get bridge.status`, `get bridge.sync`, `get mqtt.client_env` and `room.post <text>`.

## Troubleshooting

`get bridge.status` shows the bridge state, the broker, and whether repeat is on:

| State | Meaning |
| --- | --- |
| `online` | connected to the broker |
| `connecting` | trying to connect |
| `no-wifi` | waiting for Wi-Fi |
| `no-host` | no broker set; run `set bridge.peer.host` |
| `off` | `bridge.enabled` is off |

`get bridge.sync` shows `sync:<state> topic:<name> tx:<n> rx:<n> dup:<n> bad:<n> drop:<n>`. The sync state uses the same names as the bridge, plus `no-topic` when no room name is set. The counters are:

- `tx` and `rx`: posts published and received
- `dup`: duplicate deliveries, ignored
- `bad`: posts rejected, either because they failed to decrypt (usually a different `bridge.secret` on another room server) or because they are replays more than 24 hours old
- `drop`: messages discarded before processing, because they were the wrong size or arrived faster than the room server could handle them

Common problems:

- **Posts don't appear on other room servers:** check every room server has the same broker, room name and `bridge.secret`, and that `get bridge.sync` reports `online` on each.
- **`bad` keeps rising:** another room server on the same room name has a different `bridge.secret`, or a room server's clock is wrong so its posts look more than 24 hours old. Check the clock with `clock` on each, and NTP with `get ntp.server1`.
- **Bridged packets don't reach LoRa after migrating from upstream:** the board kept `repeat off`. See [Repeating](#repeating).

## Related Docs

- [Observer MQTT Bridge](./observer-mqtt-bridge.md)
- [Repeater Web API](./api.md)
- [Download and Flash Releases](./releases.md)
- [CLI Reference](./custom-cli.md#room-server-mqtt-bridge-commands)
