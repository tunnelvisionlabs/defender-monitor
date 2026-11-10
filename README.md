# Defender RO monitor

A camera-based, non-intrusive monitor for a US Water Systems Defender whole home
reverse osmosis (RO) unit, with readings available in Home Assistant.

The planned device uses a **Seeed Studio XIAO ESP32S3 Sense** and a **Seeed
Studio Round Display for XIAO**. A repeatable 3D-printed mount will hold the
camera over the Defender control panel, with a full shroud to exclude external
light. The firmware will interpret the panel on the ESP32 and report its
observations. The owner supplies the Defender and Home Assistant installation.

## Current status

The firmware implements camera acquisition and Round Display preview for the
recorded XIAO ESP32S3 Sense / OV3660 profile, with bounded ownership,
acquisition provenance, source-age handling, and local USB diagnostics.
Camera-only cold startup and five-minute camera-only and combined-driver trials
pass on the supplied assembly. Combined cold-start diagnostics also pass after
the orientation/color correction. The owner confirms upright, unmirrored
UI/camera output and correct colors with USB pointing down on the current build.
The proposed lens-to-panel target is 39–42 mm. Clean panel behavior is an
owner-accepted design assumption based on desk testing; the initial shielded
trial has not been performed. Final optical/enclosed checks remain planned, and
Windows policy currently blocks the revised preview host tests. See
[preview and optical measurements](docs/preview-and-optics.md).

Calibration, recognition, touch behavior, networking, MQTT publishing, and mount
geometry are not implemented. There is no installable Home Assistant custom
integration. See [camera capture](docs/camera-capture.md) for the implemented
contract, bring-up targets, and remaining hardware checks.

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
| `firmware/`          | Direct ESP-IDF camera bring-up application and components                            |
| `integrations/mqtt/` | MQTT and Home Assistant discovery design                                             |
| `hardware/mount/`    | Mount and shroud requirements; future editable CAD sources                           |
| `tests/`             | Camera SDK-boundary host tests and fixture policy                                    |
| `docs/`              | Architecture, hardware investigation, development, and roadmap                       |
| `local/`             | Ignored private configuration, calibration, images, and evidence; create when needed |
| `toolchain.json`     | Firmware SDK and target selection                                                    |
| `.github/workflows/` | Formatting, firmware compilation, and camera host tests                              |

## Development

Read [AGENTS.md](AGENTS.md) and the [development guide](docs/development.md).
Node and npm are development-only formatting tools; neither is a runtime
requirement for the monitor or Home Assistant.

```powershell
npm ci
npm run check
```

In a shell with the pinned ESP-IDF environment activated, compile the firmware
from the repository root on a fresh configuration:

```powershell
idf.py -C firmware build
```

Compilation does not flash a device. No setup or validation command connects to
the household installation. Firmware builds require a separately installed SDK;
see [toolchain setup](docs/development.md#firmware-toolchain), including the
configuration refresh required for an existing scaffold build.

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
