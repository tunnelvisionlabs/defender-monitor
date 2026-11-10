# Firmware

This is a direct ESP-IDF project for the ESP32-S3. `main/app_main.c` composes
the camera-only bring-up loop and local startup/progress reporting. The
`monitor_camera` component initializes the recorded OV3660 wiring and exposes
bounded, source-timestamped frame leases. No display, touch, Wi-Fi, MQTT,
private configuration, or calibration is initialized.

Use the SDK pin in `../toolchain.json` and an activated official SDK
environment:

```powershell
idf.py -C firmware build
```

Run that command from the repository root. See
[development](../docs/development.md#firmware-toolchain) for prerequisites and
generated-file handling and refreshing an existing scaffold configuration.
`sdkconfig.defaults` selects the candidate 8 MB flash, octal PSRAM, and native
USB console profile. Cold startup and five-minute camera-only capture have
passed on the supplied assembly; optical and full-system checks remain open.

Add focused native components under `components/` with their first consumers,
keeping `main/` for composition. Pin external components and retain their
dependency lockfile when introduced. The SDK's generated dependencies and build
products remain ignored.

See [camera capture](../docs/camera-capture.md) for ownership, timing, targets,
diagnostics, and the physical checklist. This is not deployment firmware.
Compilation does not establish boot or hardware acceptance and does not
authorize writing to any device.
