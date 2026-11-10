# Camera capture bring-up

The implemented path is native camera acquisition on the recorded XIAO ESP32S3
Sense / OV3660 profile. It supplies raw frames and USB diagnostics. Calibration,
recognition, Round Display preview, Wi-Fi, MQTT, and OTA are separate work.

Firmware compilation and host tests have passed. Cold startup, sustained
capture, sensor identity, optical quality, usable memory, and actual timing on
the supplied hardware remain unverified. A successful build is not physical
acceptance.

## Implemented contract

The public contract is declared in
[`monitor_camera.h`](../firmware/components/monitor_camera/include/monitor_camera.h).
One serialized owner task initializes and operates the component; this API is
not thread-safe or ISR-safe. Future concurrent consumers must coordinate with
that owner and release/copy source bytes before expensive independent work.

Initialization configures the recorded camera pins, holds the LCD and both card
selects inactive, checks PSRAM, and rejects sensors other than OV3660. It makes
one initialization attempt per boot, with no automatic retry. Failure is
reported locally and requires a restart. No credential or calibration storage is
opened.

Only one SDK-owned image can be borrowed. Its data is read-only until the same
caller-owned lease is released once; copying a live lease does not transfer
ownership. Release clears the lease and invalidates its image pointer. A slow
consumer keeps the single buffer occupied: additional acquisition returns busy
without waiting, allocating, or recording new progress. Rendering/transport
queues are not introduced here.

Every accepted acquisition includes:

- A boot-scoped 128-bit random correlation token and a delivery sequence. This
  token is neither stable device identity nor an authentication secret.
- The SDK's first-DMA-buffer timestamp, in monotonic microseconds since boot,
  and a separate delivery timestamp from the same clock. Neither is UTC.
- Explicit dimensions, byte length, and RGB565 format. Raw sensor/SDK byte order
  is preserved; display bring-up must verify color interpretation.

Identical pixels from a new source timestamp are a new acquisition. A repeated
or backwards source timestamp is rejected. Invalid dimensions, payload length,
format, missing bytes, future timestamps, and excessive source age return an
error and immediately release the SDK buffer. A timeout returns no frame.
Failure never relabels an old image as fresh.

Progress preserves the most recent successful acquisition as historical evidence
while recording failures. `ready` means initialization succeeded; it does not
establish freshness. Reading progress or printing a log does not renew an
acquisition timestamp.

## Bring-up targets and resource bounds

These choices are inputs to physical evaluation, not measurements or a Home
Assistant latency promise:

| Setting                     | Current target or enforced bound                                                         |
| --------------------------- | ---------------------------------------------------------------------------------------- |
| Driver                      | `espressif/esp32-camera` exactly 2.1.9; transitive JPEG 1.3.2 in the dependency lockfile |
| Hardware configuration      | ESP32-S3, 8 MB flash profile, octal PSRAM at 80 MHz, native USB Serial/JTAG console      |
| Capture                     | 320 by 240 RGB565, 20 MHz camera XCLK, one buffer in PSRAM                               |
| Image storage               | Exactly 153,600 bytes per frame; no application image copy or queue                      |
| Outstanding leases          | One; no acquisition progress while a consumer holds it                                   |
| Maximum accepted source age | 1,000,000 microseconds, inclusive                                                        |
| Missing-frame wait          | Pinned SDK's 4-second buffer wait per call, plus scheduling overhead; no wrapper retry   |
| Application pacing          | 100 ms delay after each acquire/release attempt; actual cadence must be measured         |
| Progress reporting          | At least 5 seconds between reports, subject to capture wait/scheduling                   |
| Main task stack             | 6,144-byte configuration target; high-water mark reported for measurement                |

The image-size bound excludes SDK DMA buffers, driver tasks, stacks, and other
heap use. Measure those costs on hardware. Current raw RGB capture is a
conservative single-buffer development choice; resolution, exposure, byte order,
and eventual Wi-Fi contention require real optical/capture trials before they
become calibrated operating settings.

The native console avoids using GPIO43/44 as UART pins when the board stack has
display connections there. The reported KE switches remain unchanged. See the
[board profile](hardware-board-profile.md) for wiring and unresolved revisions.

## Local diagnostics

Startup logs identify the project/version, build-hash prefix, SDK, chip model
and revision, SDK-reported flash size, PSRAM size, free memory, reset reason,
expected board profile, actual detected sensor PID, and boot session. They do
not log MAC addresses, credentials, images, or household configuration.

Periodic logs report total frames/failures, the last source timestamp and age,
the actual report interval and frames delivered within it, free/minimum internal
memory, free PSRAM, and main-task stack headroom. Memory values are bytes. Frame
age `-1` means no successful delivery. Compute observed cadence from interval
frames and interval microseconds, not the nominal pacing delay.

Raw logs contain runtime session data and belong in ignored `local/` storage.
Only sanitized results should be published in an issue or report.

## Physical acceptance still required

Use an explicitly authorized target and the exact built images. Routine builds,
host tests, and CI do not select, reset, flash, or provision hardware.

1. Confirm the target and expected memory/profile before flashing. Record the
   installed build and detected PID. A console message alone does not prove
   source pixels or image quality.
2. Cold-start the assembly and verify acquisition progress with the supplied
   OV3660, stock bridges, empty SD slots, and current USB supply/cable. Confirm
   the reported memory and native console behavior.
3. Run an initial five-minute capture trial, chosen before execution. Record
   actual elapsed time, counts/errors, source ages, cadence, memory/stack
   trends, supply/reset behavior, and temperature observations where measured.
   Keep an unchanged scene for part of the trial; new source timestamps must
   continue.
4. Record acquisition/init failure behavior and gaps separately from the host
   simulations. No camera-disconnection or live-installation manipulation is
   part of automatic checks.
5. Mark image visibility/segment discernibility and camera/display coexistence
   unverified until preview/optical work exercises them. Keep all unperformed
   checks explicitly open.

References:
[pinned camera driver](https://github.com/espressif/esp32-camera/tree/v2.1.9),
[frame timestamp and ownership API](https://github.com/espressif/esp32-camera/blob/v2.1.9/driver/include/esp_camera.h),
[driver buffer-wait implementation](https://github.com/espressif/esp32-camera/blob/v2.1.9/driver/esp_camera.c),
and
[ESP-IDF monotonic time](https://docs.espressif.com/projects/esp-idf/en/v5.5.5/esp32s3/api-reference/system/esp_timer.html).
