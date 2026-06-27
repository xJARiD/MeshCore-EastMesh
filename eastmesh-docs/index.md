# MeshCore EastMesh Docs

MeshCore-EastMesh keeps the upstream MeshCore firmware intact and publishes five firmware tracks, depending on how the device needs to connect:

- `companion-wifi`: use this for Wi-Fi-connected companion devices. It stays closest to upstream MeshCore and adds the EastMesh Wi-Fi rescue/configuration commands.
- `repeater-bridge-espnow`: use this when you need a plain upstream-style repeater ESP-NOW bridge without MQTT uplink or the EastMesh web panel.
- `observer-eastmesh`: use this for a Wi-Fi repeater that should publish to an MQTT broker and, on supported ESP32 boards, offer the optional local web panel for setup and troubleshooting.
- `observer-eastmesh-bridge-espnow`: use this when one repeater needs both MQTT uplink and ESP-NOW bridge duties, including bridge channel/secret controls for keeping the bridge aligned with Wi-Fi.
- `observer-eastmesh-bridge-mqtt`: use this when one repeater needs both MQTT uplink and bidirectional MQTT mesh bridging through a peer broker you configure in the web panel or CLI.

!!! note "Bridge tracks are local radio bridges"

    The ESP-NOW bridge tracks are for bridging two nearby repeaters that operate on different MeshCore radio configs, for example one repeater on `Australia (Narrow)` and another on `Australia (Mid)`.

    The MQTT bridge track uses a shared topic at a peer MQTT broker for mesh packet bridging. It is separate from MQTT uplink publishing to EastMesh or MeshMapper.

    Bridge tracks are not MQTT-over-WAN, VPN, or internet tunnel releases.

If you want guidance first, start with:

- [Compare Boards](./boards.md)
- [Download and Flash Releases](./releases.md)

If you already know your board and just want the quickest path, skip the docs and open the flasher:

- [Open the EastMesh Flasher](https://flasher.eastmesh.au)

## I Want To

- choose a board: start with [Compare Boards](./boards.md)
- flash firmware with guidance: start with [Download and Flash Releases](./releases.md)
- flash firmware now: open the [EastMesh Flasher](https://flasher.eastmesh.au)
- set up a repeater after flashing: use [Download and Flash Releases](./releases.md) and [Use the Repeater Web Panel](./web-panel.md)
- understand EastMesh CLI commands: use [Custom CLI Commands](./custom-cli.md)
- automate or script against a repeater: use [Use the Repeater Web API](./api.md)
- build firmware locally: use [Build Locally With uv](./local-builds.md)

## End User Guides

- [Compare Boards](./boards.md)
- [Download and Flash Releases](./releases.md)
- [Migration From xJARiD/MeshCore](./migration.md)
- [Use the Repeater Web Panel](./web-panel.md)
- [Use the Repeater Web API](./api.md)
- [Custom CLI Commands](./custom-cli.md)

## Developer Notes

- [Build Locally With uv](./local-builds.md)

## Current Scope

This docs site only covers the EastMesh-specific pieces in this repository.

For general MeshCore behaviour, radio operation, and upstream firmware concepts, refer to the upstream project:

- [meshcore-dev/MeshCore](https://github.com/meshcore-dev/MeshCore)

## Australian Regional References

These references are community-maintained rather than official project documentation, but they are still useful for Australian regional setup notes, local conventions, and area-specific MeshCore references.

### ACT / NSW / QLD / SA / TAS / VIC

- [wiki.eastmesh.au](https://wiki.eastmesh.au/)
- [wiki.meshcoreaus.org](https://wiki.meshcoreaus.org/)

### Sydney

- [nswmesh.au](https://nswmesh.au/)
- [github.com/nswmesh](https://github.com/nswmesh/)

### Brisbane

- [wiki.mbug.com.au/en/Meshcore/Settings](https://wiki.mbug.com.au/en/Meshcore/Settings)
