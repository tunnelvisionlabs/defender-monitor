# Development

## Formatting tools

Use Node 24 and npm 11. `.node-version` records Node 24.12.0, `package.json`
records npm 11.6.2, and `package-lock.json` locks the development dependencies.
Prettier 3.9.10 formats supported Markdown, JSON, and YAML files. Node is used
only for repository tooling.

From the repository root:

```powershell
npm ci
npm run check
```

`check` runs formatting verification only. Camera SDK-boundary tests have their
own [host test commands](../tests/README.md). Formatting does not compile
firmware, validate CAD, or contact a device.

`npm run format` formats supported files throughout the repository. For a
focused edit, pass explicit paths instead:

```powershell
npm exec -- prettier --write README.md docs/architecture.md
```

Review resulting changes and run the appropriate checks. Neither formatting
command changes the Git index. Preserve unrelated changes; broad formatting is
not a routine prerequisite for a narrow change. C, CMake, CAD, and SDK settings
are outside Prettier's supported scope here; use EditorConfig and review them
with their relevant build/design tools.

## Firmware toolchain

Install **ESP-IDF 5.5.5** with its **ESP32-S3** tools using Espressif's
[installation instructions](https://docs.espressif.com/projects/esp-idf/en/v5.5.5/esp32s3/get-started/index.html).
The fixed SDK baseline is recorded in `toolchain.json`; it is not a claim to
track the newest SDK release. Do not substitute a moving branch. Use the SDK's
supported Python environment rather than an unrelated global Python install.

On Windows, use the installed ESP-IDF PowerShell environment, or dot-source that
installation's `export.ps1`. On Linux/macOS, source its `export.sh`. Keep local
SDK paths outside tracked files. The repository does not install global tools or
create an SDK environment automatically.

From that activated environment, run from the repository root:

```powershell
idf.py --version
idf.py -C firmware build
```

The CMake project reads the SDK/target pin from `toolchain.json`, checks the
SDK's numeric version, and rejects a different CMake target. Build outputs and
generated `sdkconfig` stay under `firmware/` and are ignored.
`sdkconfig.defaults` is tracked and selects the candidate XIAO Sense camera
profile: 8 MB flash, octal PSRAM, and a native USB Serial/JTAG console. The
camera manifest pins its driver exactly, and `firmware/dependencies.lock` pins
the resolved component versions and hashes. Keep the lockfile tracked;
`managed_components/` is generated. The current partition table remains the SDK
default; OTA layout and distribution belong to their own implementation step.

An existing `firmware/sdkconfig` overrides changed defaults. Refresh an old
scaffold build using a new ignored configuration/build directory, preserving the
old generated files:

```powershell
$captureBuild = Join-Path (Get-Location) ".cache/firmware-capture"
$captureConfig = Join-Path $captureBuild "sdkconfig"
idf.py -C firmware -B $captureBuild -D "SDKCONFIG=$captureConfig" build
```

Run this from the repository root with the SDK activated. Absolute paths avoid
ambiguity about which directory resolves `-B`. The application rejects a build
without the required PSRAM, flash-size, and native-USB settings. Regenerate the
chosen configuration when intentionally changing defaults; do not silently use a
previously generated scaffold configuration. Camera resolution and exposure
remain provisional bring-up choices until physical trials establish them.

The
[ESP-IDF build-system guide](https://docs.espressif.com/projects/esp-idf/en/v5.5.5/esp32s3/api-guides/build-system.html)
describes project/component organization and command behavior. Building is a
local compile step. Flashing, provisioning, publishing MQTT messages, and
changing Home Assistant are separate operations requiring an authorized target.

## Private workspace

Create ignored `local/` storage when needed. Suggested subdirectories are
`local/devices/`, `local/calibration/`, `local/captures/`, and
`local/validation/`. These are conventions, not a configuration format or a
loader implemented by the scaffold. Do not add credentials or realistic
household identifiers to tracked examples. Scrub logs and images before
including any in public evidence.

Keep generated build/check output in ignored `build/`, `.cache/`, `output/`, or
`artifacts/` paths. Keep editable CAD models tracked and generated print exports
under ignored `hardware/mount/exports/` during development.

## Continuous integration

The verification workflow checks formatting on Windows and Linux and separately
compiles the camera application and runs host capture tests in
`espressif/idf:v5.5.5`. It reads `toolchain.json` for the SDK selection and uses
immutable GitHub Action revisions. It does not flash, discover devices, publish
releases, or access private files. A configured workflow is not evidence of a
successful remote run.

Add relevant build, lint, or test commands as their first implementation
arrives. Keep [testing guidance](testing.md) and status documentation accurate;
avoid empty test jobs or successful checks that imply unimplemented features
work.
