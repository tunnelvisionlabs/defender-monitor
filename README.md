# Defender RO monitor

A camera-based, non-intrusive monitor for a US Water Systems Defender whole home
reverse osmosis (RO) unit, with readings available in Home Assistant.

The planned device uses a **Seeed Studio XIAO ESP32S3 Sense** and a **Seeed
Studio Round Display for XIAO**. A repeatable 3D-printed mount will hold the
camera over the Defender control panel, with a full shroud to exclude external
light. The firmware will interpret the panel on the ESP32 and report its
observations. The owner supplies the Defender and Home Assistant installation.

## Current status

This repository contains development foundations. The ESP-IDF application entry
point is intentionally empty. Camera capture, calibration, recognition, display
and touch behavior, networking, MQTT publishing, and mount geometry are not
implemented. There is no installable Home Assistant custom integration.

The initial Home Assistant connection will use MQTT discovery through Home
Assistant's existing MQTT integration. A broker is an additional deployment
prerequisite. Add a custom integration only if a demonstrated requirement cannot
be met adequately through MQTT.

This is an observation project: the planned monitor will not operate the RO
controls or modify its electrical connections or plumbing. Camera observations
describe what the panel shows; they do not independently measure water quality.

## Repository layout

| Path                 | Purpose                                                                              |
| -------------------- | ------------------------------------------------------------------------------------ |
| `firmware/`          | Minimal direct ESP-IDF application and future device components                      |
| `integrations/mqtt/` | MQTT and Home Assistant discovery design                                             |
| `hardware/mount/`    | Mount and shroud requirements; future editable CAD sources                           |
| `tests/`             | Test organization and fixture policy; no feature tests yet                           |
| `docs/`              | Architecture, hardware investigation, development, and roadmap                       |
| `local/`             | Ignored private configuration, calibration, images, and evidence; create when needed |
| `toolchain.json`     | Firmware SDK and target selection                                                    |
| `.github/workflows/` | Formatting checks and compilation of the empty firmware                              |

## Development

Read [AGENTS.md](AGENTS.md) and the [development guide](docs/development.md).
Node and npm are development-only formatting tools; neither is a runtime
requirement for the monitor or Home Assistant.

```powershell
npm ci
npm run check
```

In a shell with the pinned ESP-IDF environment activated, compile the scaffold
from the repository root:

```powershell
idf.py -C firmware build
```

Compilation does not flash a device. No setup or validation command connects to
the household installation. Firmware builds require a separately installed SDK;
see [toolchain setup](docs/development.md#firmware-toolchain).

## Project notes

- [Architecture and boundaries](docs/architecture.md)
- [Hardware facts and open questions](docs/hardware.md)
- [MQTT integration direction](integrations/mqtt/README.md)
- [Mount and shroud requirements](hardware/mount/README.md)
- [Testing and evidence](docs/testing.md)
- [Development roadmap](docs/roadmap.md)
- [Contribution and writing guidance](docs/contributing.md)

### Local Codex initialization prompt

> Read AGENTS.md and README.md to familiarize yourself with this repository.
> After work that changes repository files, provide a suggested commit message
> in a fenced code block. Leave changes unstaged unless staging is explicitly
> requested; creating a commit requires separate explicit authorization.
