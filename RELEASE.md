# Releasing Firmware

GitHub Actions is set up to automatically build and draft firmware releases.

## Required GitHub Variable

Set the repository Actions variable `OFFICIAL_MESHCORE_VERSION` to the current upstream MeshCore release version.

Example:

- `OFFICIAL_MESHCORE_VERSION=v1.15.0`

This value becomes the base `FIRMWARE_VERSION` used by the release workflows.

## EastMesh Release Tags

Push one or more of the following tag formats to trigger the matching firmware release workflow:

- `repeater-bridge-espnow-v1.15.0`
- `observer-eastmesh-bridge-espnow-v2026.5.1`
- `observer-eastmesh-bridge-mqtt-v2026.7.0`
- `observer-eastmesh-v2026.5.1`
- `room-server-eastmesh-bridge-mqtt-v2026.10.1`

Use the upstream MeshCore version in `repeater-bridge-espnow-v1.15.0`.
Use the EastMesh release version in `observer-eastmesh-bridge-espnow-v2026.5.1`.
Use the EastMesh release version in `observer-eastmesh-bridge-mqtt-v2026.7.0`.
Use the EastMesh release version in `observer-eastmesh-v2026.5.1`.
Use the EastMesh release version in `room-server-eastmesh-bridge-mqtt-v2026.10.1`.

Each tag triggers a separate workflow:

- `repeater-bridge-espnow-v*` builds repeater ESP-NOW bridge firmware
- `observer-eastmesh-bridge-espnow-v*` builds Observer ESP-NOW firmware
- `observer-eastmesh-bridge-mqtt-v*` builds Observer MQTT bridge firmware
- `observer-eastmesh-v*` builds Observer firmware
- `room-server-eastmesh-bridge-mqtt-v*` builds Room Server MQTT bridge firmware (`*_room_server_mqtt_bridge`)

You can push one, or more tags on the same commit, and they will all build separately.

## Resulting Firmware Version

During the GitHub Actions build:

- `repeater-bridge-espnow` uses the version in the tag as `FIRMWARE_VERSION`
- `observer-eastmesh-bridge-espnow` uses `OFFICIAL_MESHCORE_VERSION` as `FIRMWARE_VERSION` and the EastMesh version from the tag as `EASTMESH_VERSION`
- `observer-eastmesh-bridge-mqtt` uses `OFFICIAL_MESHCORE_VERSION` as `FIRMWARE_VERSION` and the EastMesh version from the tag as `EASTMESH_VERSION`
- `observer-eastmesh` uses `OFFICIAL_MESHCORE_VERSION` as `FIRMWARE_VERSION` and the EastMesh version from the tag as `EASTMESH_VERSION`
- `room-server-eastmesh-bridge-mqtt` uses `OFFICIAL_MESHCORE_VERSION` as `FIRMWARE_VERSION` and the EastMesh version from the tag as `EASTMESH_VERSION`

The resulting version string depends on the workflow:

- `repeater-bridge-espnow`: `v1.15.0-<commit>`
- `observer-eastmesh-bridge-espnow`: `v1.15.0-eastmesh-v2026.5.1-<commit>`
- `observer-eastmesh-bridge-mqtt`: `v1.15.0-eastmesh-v2026.7.0-<commit>`
- `observer-eastmesh`: `v1.15.0-eastmesh-v2026.5.1-<commit>`
- `room-server-eastmesh-bridge-mqtt`: `v1.15.0-eastmesh-v2026.10.1-<commit>`

Example:

- tag: `observer-eastmesh-v2026.5.1`
- repo variable: `OFFICIAL_MESHCORE_VERSION=v1.15.0`
- resulting build version: `v1.15.0-eastmesh-v2026.5.1-abcdef`

## Typical Release Flow

1. Update the `OFFICIAL_MESHCORE_VERSION` GitHub Actions variable if upstream has changed.
2. Update `release-notes.yml` on `develop` for any release entries you want included in the tagged commit.
3. Open and merge the release PR from `develop` to `main`.
4. Create the release tags you want on the target commit on `main`.
5. Push the tags.
6. Wait for the workflows to finish building.
7. Review the draft GitHub Releases.
8. Publish the release.

If `release-notes.yml` should reflect the tagged firmware release in-repo, make that change before the PR to `main` so the tagged commit already contains the matching release notes.

Example:

```bash
git tag repeater-bridge-espnow-v1.15.0
git tag observer-eastmesh-bridge-espnow-v2026.5.1
git tag observer-eastmesh-bridge-mqtt-v2026.7.0
git tag observer-eastmesh-v2026.5.1
git tag room-server-eastmesh-bridge-mqtt-v2026.10.1
git push origin repeater-bridge-espnow-v1.15.0 observer-eastmesh-bridge-espnow-v2026.5.1 observer-eastmesh-bridge-mqtt-v2026.7.0 observer-eastmesh-v2026.5.1 room-server-eastmesh-bridge-mqtt-v2026.10.1
```

## Supported Tags

- `repeater-bridge-espnow-v1.15.0`
- `observer-eastmesh-bridge-espnow-v2026.5.1`
- `observer-eastmesh-bridge-mqtt-v2026.7.0`
- `observer-eastmesh-v2026.5.1`
- `room-server-eastmesh-bridge-mqtt-v2026.10.1`

The `companion-wifi` track has been retired. Upstream MeshCore now ships `*_companion_radio_wifi` firmware with persisted Wi-Fi settings, so EastMesh no longer builds or tags companion Wi-Fi releases.
