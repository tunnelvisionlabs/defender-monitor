# Architecture

## Intended data flow

```text
Defender control panel
        | light inside a fixed, shrouded mount
        v
ESP32: capture -> calibrated interpretation -> observation
                                              |
                                  MQTT state and discovery
                                              v
                                         MQTT broker
                                              v
                              Home Assistant MQTT integration
                                              v
                                Defender device and entities
```

This describes the intended system. Today, `firmware/main/app_main.c` is an
empty entry point; the remaining boxes have no implementation in this
repository.

## Responsibilities

| Boundary                | Responsibility when implemented                                                                      |
| ----------------------- | ---------------------------------------------------------------------------------------------------- |
| Board support           | Pin assignments, camera/display/touch drivers, and hardware initialization                           |
| Capture and calibration | Frame acquisition, camera settings, panel geometry, and calibration validity                         |
| Interpretation          | Convert calibrated image regions into observations without network dependencies                      |
| Observation model       | Identify the observed field, validity, freshness, and provenance independently of transport          |
| MQTT adapter            | Encode observations, device discovery, connection status, and reconnect behavior                     |
| Local display           | Present monitor status and eventual calibration feedback without owning recognition or network logic |
| Mount and shroud        | Hold the optical geometry and exclude external light while preserving service access                 |

Keep application composition in `firmware/main/`. Introduce focused ESP-IDF
components under `firmware/components/` when the first implementation needs
them. Do not create empty component APIs or allocate tasks before their
requirements are known.

## Project decisions

Use native ESP-IDF with explicit SDK/component pins. The initial compile target
is ESP32-S3; camera pins, flash/PSRAM tuning, and display drivers remain subject
to verification against the actual board revisions.

Interpret the panel on the ESP32. Routine reporting should transmit observations
rather than a continuous camera stream. Diagnostic image capture and retention
need an explicit design; private images belong in ignored storage. The camera
microphone is outside the monitoring scope.

Begin with Home Assistant's existing MQTT integration and discovery. The monitor
will represent the Defender as a device with verified observable fields. Keep
monitor health distinguishable from RO state. The MQTT design lives in
[the integration notes](../integrations/mqtt/README.md); a custom Home Assistant
component remains deferred.

## Decisions that need evidence

Panel inventory and representative captures must precede entity names, units,
and recognition algorithms. Region sampling, segment decoding, templates, OCR,
and learned models are possible approaches, not selected implementations. Start
with the least complex approach that meets measured reliability needs.

Choose capture resolution, exposure, cadence, resource budgets, and temporal
filtering after testing the actual panel. Account for panel refresh/flicker,
transitions, and incomplete frames. A shroud excludes ambient light but does not
by itself prove focus, exposure, or sufficient illumination.

Define freshness deadlines, invalid-reading behavior, and restart recovery
before publishing entities. Connectivity, observation validity, and the last
confirmed RO state are separate concepts. Neither a connected broker nor a
retained message proves that the panel is currently readable.

Provisioning, credential storage, firmware updates, and recovery need their own
designs before deployment. The scaffold chooses no partition scheme for OTA,
network management API, provisioning method, or persistent calibration schema.
