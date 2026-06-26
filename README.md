# MeshCore-EastMesh

MeshCore-EastMesh originates from the upstream MeshCore project:

- <https://github.com/meshcore-dev/MeshCore>

Credits to Scott Powell / Ripple Radios and the MeshCore contributors for the original firmware and project foundation.

EastMesh is a community-driven mesh network across Eastern Australia. This repository provides tailored builds of MeshCore, including WiFi companion firmware with additional CLI tools and Observer firmware that integrates with our CoreScope telemetry platform.

CoreScope (<https://core.eastmesh.au>) offers visibility into the network, including repeater status, observers, and mapping data.

**Observer builds feed EastMesh Core telemetry and are intended for Eastern Australia repeaters only**; running them elsewhere can skew shared network data.

## What This Repo Adds

- `*_repeater_observer` firmware targets with:
  - native WiFi
  - MQTT over WSS with JWT auth
  - optional local HTTPS config panel on supported ESP32 targets
- `*_companion_radio_wifi` firmware targets for WiFi-connected companion devices
- EastMesh-specific release workflows and versioning on top of upstream MeshCore releases
- docs and release guidance for EastMesh users instead of the full upstream MeshCore docs set

## Releases

Prebuilt firmware is published on GitHub Releases:

- <https://github.com/xJARiD/MeshCore-EastMesh/releases>

For flashing guidance, including when to use `.bin` vs `-merged.bin`, see:

- [eastmesh-docs/releases.md](./eastmesh-docs/releases.md)

The custom firmware flasher site is:

- <https://flasher.eastmesh.au/>

## Install uv

This repo uses `uv` for Python tooling and runs PlatformIO through `uv run`.

Official install docs:

- <https://docs.astral.sh/uv/getting-started/installation/>

Common install methods:

macOS and Linux:

```bash
curl -LsSf https://astral.sh/uv/install.sh | sh
```

Homebrew:

```bash
brew install uv
```

Windows PowerShell:

```powershell
powershell -ExecutionPolicy ByPass -c "irm https://astral.sh/uv/install.ps1 | iex"
```

## Common Repo Usage

Install tooling:

```bash
uv sync
```

List available build targets:

```bash
bash eastmesh-build.sh list
```

Build a single target:

```bash
uv run pio run -e heltec_v4_repeater_observer
uv run pio run -e heltec_v4_companion_radio_wifi
```

Build with release-style version metadata:

```bash
export FIRMWARE_VERSION=v1.15.0
export EASTMESH_VERSION=v2026.5.1
bash eastmesh-build.sh build-firmware heltec_v4_repeater_observer
```

Flash a target:

```bash
uv run pio run -e heltec_v4_repeater_observer -t upload --upload-port /dev/tty.usbmodemXXXX
```

Open a serial monitor:

```bash
uv run pio device monitor --port /dev/tty.usbmodemXXXX --baud 115200
```

Build docs locally:

```bash
uv run --group docs zensical serve
uv run --group docs zensical build
```

## Key Files

- [`eastmesh-build.sh`](./eastmesh-build.sh)
  - EastMesh local/release build wrapper
  - injects `FIRMWARE_VERSION`, `CLIENT_VERSION`, and EastMesh release metadata
- [`build.sh`](./build.sh)
  - upstream MeshCore build wrapper retained to reduce future merge conflicts
- [`pyproject.toml`](./pyproject.toml)
  - Python tooling and docs dependencies
- [`platformio.ini`](./platformio.ini)
  - root PlatformIO config and ESP32 helper scripts
- [`variants/eastmesh_mqtt/platformio.ini`](./variants/eastmesh_mqtt/platformio.ini)
  - shared EastMesh observer env definitions
- [`examples/simple_repeater/MyMesh.cpp`](./examples/simple_repeater/MyMesh.cpp)
  - repeater CLI wiring, MQTT command surface, and web allowlist integration
- [`src/helpers/mqtt/MQTTUplink.cpp`](./src/helpers/mqtt/MQTTUplink.cpp)
  - MQTT uplink implementation, HTTPS web panel, WSS/JWT handling, and repeater WiFi control
- [`examples/companion_radio`](./examples/companion_radio)
  - companion firmware implementation
- [`RELEASE.md`](./RELEASE.md)
  - tag formats and release workflow behavior
- [`eastmesh-docs/`](./eastmesh-docs)
  - EastMesh-focused docs published to GitHub Pages
- [`docs/`](./docs)
  - upstream MeshCore docs retained to reduce future merge conflicts

## Key EastMesh Features

### MQTT Repeater Additions

- curated broker support for:
  - `eastmesh-au`
  - `meshmapper`
  - `waev`
  - `letsmesh-eu` (retired)
  - `letsmesh-us` (retired)
- WSS transport at `/mqtt`
- JWT auth using the device identity
- CLI controls for:
  - WiFi credentials
  - WiFi powersaving
  - board battery reporting on supported targets
  - MQTT endpoint enablement
  - MQTT packet and raw publishing
  - owner public key and email
  - local web panel enablement

### Local Web Panel

On supported `*_repeater_observer` ESP32 targets, the repeater can expose a local HTTPS config panel over WiFi.

Features include:

- password-gated access using the existing repeater admin password
- allowlisted CLI execution
- grouped quick actions
- light and dark themes
- optional disable via `set web off`

Recommended use is initial setup and occasional troubleshooting. On observer deployments, disable the panel again when finished if you want maximum MQTT heap headroom.

### Companion WiFi Additions

`*_companion_radio_wifi` targets now support persisted WiFi rescue commands:

- open a serial monitor at `115200` baud
- reboot the device
- long-press the user button within the first 8 seconds after boot to enter `CLI Rescue`
- wait for `========= CLI Rescue =========`
- send the commands from the serial monitor

These rescue commands are only available after entering `CLI Rescue`:

- `get wifi.status`
- `get wifi.ssid`
- `get wifi.powersaving`
- `set wifi.ssid <ssid>`
- `set wifi.pwd <password>`
- `set wifi.powersaving none|min|max`

## Active GitHub Workflows

- `.github/workflows/eastmesh-build-companion-wifi-firmwares.yml`
- `.github/workflows/eastmesh-build-observer-firmwares.yml`
- `.github/workflows/eastmesh-build-repeater-bridge-espnow-firmwares.yml`
- `.github/workflows/eastmesh-build-observer-espnow-firmwares.yml`
- `.github/workflows/eastmesh-pr-build-check.yml`
- `.github/workflows/eastmesh-push-build-check.yml`
- `.github/workflows/eastmesh-github-pages.yml`

The upstream MeshCore workflows are retained under their original filenames to reduce merge conflicts. They are not part of the EastMesh release flow and should stay disabled in GitHub Actions for this repository.

The current release workflows intentionally focus only on:

- `companion-wifi`
- `repeater-bridge-espnow`
- `observer-eastmesh`
- `observer-eastmesh-bridge-espnow`

## Release Tags

Current release tags are:

```bash
git tag companion-wifi-v1.14.1
git tag repeater-bridge-espnow-v1.15.0
git tag observer-eastmesh-v2026.5.1
git tag observer-eastmesh-bridge-espnow-v2026.5.1
```

Companion WiFi uses the upstream MeshCore version in the tag.

Observer uses:

- `OFFICIAL_MESHCORE_VERSION` from GitHub Actions variables as `FIRMWARE_VERSION`
- the EastMesh tag version as `EASTMESH_VERSION`

See [RELEASE.md](./RELEASE.md) for the full release flow.

## Documentation

Published docs site:

- <https://xjarid.github.io/MeshCore-EastMesh/>

Current docs pages:

- [Home](./eastmesh-docs/index.md)
- [Download and Flash Releases](./eastmesh-docs/releases.md)
- [Build Locally With uv](./eastmesh-docs/local-builds.md)
- [Custom CLI Commands](./eastmesh-docs/custom-cli.md)
