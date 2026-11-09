# Firmware

This is a direct ESP-IDF project for the ESP32-S3. `main/app_main.c` contains an
empty entry point so the SDK/target can be compiled before device features are
introduced. It does not initialize camera, display, touch, Wi-Fi, MQTT, private
configuration, or calibration.

Use the SDK pin in `../toolchain.json` and an activated official SDK
environment:

```powershell
idf.py -C firmware build
```

Run that command from the repository root. See
[development](../docs/development.md#firmware-toolchain) for prerequisites and
generated-file handling. `sdkconfig.defaults` intentionally leaves
board-specific flash, PSRAM, and peripheral configuration undecided.

Add focused native components under `components/` with their first consumers,
keeping `main/` for composition. Pin external components and retain their
dependency lockfile when introduced. The SDK's generated dependencies and build
products remain ignored.

This scaffold is not deployment firmware. Compilation does not establish boot or
hardware acceptance and does not authorize writing to any device.
