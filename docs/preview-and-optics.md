# Round Display preview and optical measurements

The implemented preview uses native `esp_lcd` with `espressif/esp_lcd_gc9a01`
exactly 2.0.4. The dependency lock also fixes `cmake_utilities` 0.5.3. It adds
no Arduino, LVGL, touch, storage, networking, calibration, or recognition.
Firmware compilation, 49 host scenarios, and a five-minute combined-driver bench
trial pass. Visible display output, combined cold startup, and panel
measurements remain pending; transfer completion alone does not verify them.

## Preview behavior

The entire 320 by 240 camera frame is sampled into a 192 by 144 preview at
screen coordinates (24, 48). This preserves its 4:3 aspect ratio inside the
240-pixel circular display, including the source corners. It does not reduce the
source frame or select calibrated crops. Small segments that disappear in the
preview might still be present in the source; preview readability cannot
establish recognition accuracy.

The screen has an `UP` marker, `L`/`R` markers, a frame number, and source age
at draw preparation in milliseconds. A bottom strip contains red, green, and
blue in that order. Use these to verify orientation, mirroring, and color on the
actual panel. The initial LCD configuration is no XY swap or mirroring, RGB
element order, MSB-first RGB565, and inversion enabled. These are candidate
settings until visually confirmed. Record the USB connector's direction with the
text upright before CAD or calibration assigns a top edge.

The preview contains camera evidence, with no interpreted Defender status.
`WAITING`, `STALE`, and `CAMERA ERROR` views contain no source image. A failed
acquisition invalidates a queued image. Source age above one second makes a
preview stale; status reads, delivery time, and rendering do not renew source
time. Identical pixels from a genuinely new acquisition still have a new frame
number. The frame's boot token, sequence, and acquisition time survive handoff.

## Ownership, concurrency, and limits

The
[public API](../firmware/components/monitor_preview/include/monitor_preview.h)
declares the implemented contracts. `app_main` remains the sole camera owner. It
scales/copies a borrowed camera frame into the fixed preview pool and then
releases the SDK lease. A separate display-owner task performs all LCD calls.
Only slot bookkeeping and progress snapshots take a critical section; neither
pixel copying nor SDK LCD calls hold that lock.

Three slots separate an in-progress producer copy, a pending image, and a
consumer copy. There is at most one pending image; a newer submission replaces
it. A slow or blocked display does not wait in the producer or hold the camera
lease. The consumer copies into a distinct DMA canvas and releases the preview
slot before submitting the transfer. Buffers are allocated once at startup;
there are no per-frame allocations or unbounded image queues.

| Setting              | Target or enforced bound; not a hardware measurement                                      |
| -------------------- | ----------------------------------------------------------------------------------------- |
| Preview image pool   | Three 192 by 144 RGB565 slots: 165,888 bytes in PSRAM                                     |
| LCD canvas           | One 240 by 240 RGB565 canvas: 115,200 bytes of internal DMA memory                        |
| LCD SPI              | SPI2, mode 0, 20 MHz; clock GPIO7, MOSI GPIO9, CS GPIO2, DC GPIO4; no MISO or GPIO reset  |
| Shared cards         | GPIO3 and GPIO21 held inactive/high; no SD driver; GPIO21 is also the active-low user LED |
| Backlight            | Reported KE poles remain open; no GPIO43 backlight control                                |
| Transfer queue       | One native color transaction, at most one outstanding canvas                              |
| Display task         | 6,144-byte stack target; step every 50 ms when SDK calls return                           |
| Source freshness     | 1,000,000 microseconds inclusive, measured from acquisition                               |
| Transfer timeout     | 500 ms; retain DMA ownership and count one timeout per transfer                           |
| Initialization retry | Five seconds after a failed native init; retry only when cleanup permits                  |

These image bytes exclude SDK descriptors, camera storage, task stacks, and
other heap costs. Measure free/minimum memory and stack headroom with both
drivers running. A 20 MHz SPI target does not establish a measured preview rate.

The DMA canvas stays immutable until the native completion callback, including
after a timeout. A late completion permits use of the latest source. A rejected
draw is not counted as rendered; the next fresh source can retry. Initialization
failure retries without accumulating image allocations. Failed native cleanup
requires restart. The pinned GC9A01/SPI path queues the entire canvas as one
color transaction; its error-before-queue behavior is the ownership boundary.

SPI has no visible-output acknowledgment. A completed transfer means the SDK
released the bytes, not that a person verified the LCD. Native command or
initialization calls can block; they are isolated to the display task. A stuck
LCD cannot show an updated stale/error warning. Its frozen frame counter and
age-at-draw must not be interpreted as a working monitor watchdog. Capture can
continue while that display is unavailable.

## Local diagnostics and checks

The existing camera report remains unchanged. A second periodic USB report
records submitted/replaced/expired previews, completed live transfers, draw
failures/timeouts, last rendered source time, maximum producer copy window,
display readiness, pending transfer, selected view, display error, and display
task stack headroom. `max_submit_us` measures from the producer's initial clock
sample through reservation/scaling, before final publication bookkeeping; it
does not measure the entire API call. Times are microseconds and memory/stack
values are bytes. View numbers are 0 waiting, 1 live, 2 stale, and 3 capture
error. These logs belong in ignored `local/` storage.

The [host suite](../tests/README.md) executes production scaling, composition,
mailbox ownership, expiry, and recovery with fakes for the clock, allocation,
critical sections, and LCD transfer boundary. The LCD fake enforces immutable
borrowed bytes and delayed completion. Tests preserve source corner/color
markers, reuse camera bytes immediately after submit, and hold DMA while
hundreds of newer source frames arrive. They do not emulate SPI, SDK semaphore
interrupts, real task scheduling, or sensor/optical behavior. Native firmware
compilation and authorized physical trials cover different evidence.

## Bench evidence: 2026-11-09

The supplied assembly remained on its current computer USB supply/cable, with
the Round Display attached, both reported KE poles open, both SD slots empty,
and no networking or hardware modification. The installed app image SHA-256 is
`4d8e9ece9f303cfddd6d3f0265887dd557cbe28ab0237dc6a2eca5480fd28784`; its startup
ELF prefix is `affdadc6e955a4ea`. Installation passed write/hash verification.
Startup reports PID `0x3660`, 8 MB flash, and an 8 MB octal PSRAM device at 80
MHz passing the SDK memory test.

| Check                      | Observed result                                                                                                                                                               |
| -------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Trial                      | Preselected 300-second run after USB restart; actual recording 300.109 seconds, with 59 complete camera and 59 preview reports                                                |
| Final periodic report      | At 298.047 seconds: 1,651 captures/submissions, 1,650 completed live preview transfers, zero capture failures, replacements, expirations, draw failures, or transfer timeouts |
| Cadence and progress       | Both source timestamp series increase throughout; capture and preview transfer cadence across source reports are approximately 5.553 per second                               |
| Reported camera source age | 97,759–97,801 microseconds, sampled after the preview handoff                                                                                                                 |
| Producer copy window       | Maximum reported `max_submit_us` is 7,726 microseconds                                                                                                                        |
| Internal heap              | 191,651 bytes free throughout reports; minimum since boot 159,620 bytes                                                                                                       |
| PSRAM                      | 8,063,560 bytes free throughout reports                                                                                                                                       |
| Stack headroom             | Main task minimum free 4,044 bytes; display task minimum free 4,168 bytes, each from a 6,144-byte configured stack                                                            |
| Runtime diagnostics        | One application startup, no logged SDK error, panic, brownout, or camera overflow; display ready and selected live view in every report                                       |

Counts stop at periodic snapshots; the capture/transfer difference at that
snapshot is not an endpoint loss measurement. The receive path retained a
partial pre-reset line; analysis begins at the recorded new application boot and
uses only complete reports. Raw logs remain in ignored `local/` storage. These
measurements do not establish visible pixels, maximum per-frame latency,
voltage/current margins, temperatures, or long-term/enclosed reliability. No
physical camera/display fault was induced. Recovery and delayed-DMA ownership
have host coverage; actual fault recovery remains unmeasured.

Visible output/color/orientation, combined cold startup, and the optical
envelope at the Defender remain open. The worksheet below supports panel work on
USB power away from this computer.

## Bench acceptance procedure and remaining checks

1. Install the exact built image on an authorized target. Confirm native
   startup, PSRAM, the sensor, display readiness, and allocation success.
2. Verify readable text, the complete live image, and red/green/blue bar order.
   Move a recognizable object left/right and upward to check orientation and
   mirroring. Record where the USB connector points when `UP` is upright.
3. Observe a stationary scene for a preselected five-minute trial. Record actual
   capture/preview cadence, replacement/errors, source age, maximum copy time,
   memory/stack trends, and resets. Console success alone does not verify
   pixels.
4. Remove USB power for at least five seconds, reconnect, and verify combined
   camera/display cold startup and a moving preview without button access.
5. Keep simulated failure coverage separate from any physical fault trial. Do
   not disconnect the camera or change board switches as an automatic check.

## Panel measurement worksheet

The Defender is away from the development computer. Use a USB power source at
the panel and record observations on paper or another device, then copy them
back. Wi-Fi is not required for this step. A temporary repeatable fixture and
light shield should hold the camera still without final enclosure geometry.

For each trial, record the following. Distances refer to the front of the camera
lens and the control-panel face, along the viewing direction. Do not substitute
PCB spacing for working distance.

| Field                 | Observation to record                                                                                                                         |
| --------------------- | --------------------------------------------------------------------------------------------------------------------------------------------- |
| Supply and setup      | USB source rating, cable, fixture/shield arrangement, and whether the scene stayed still                                                      |
| Orientation           | Connector direction with screen text upright; camera image orientation and any observed mirroring relative to panel text                      |
| Working distance      | Measured millimeters from lens front to panel face; measurement method and approximate uncertainty                                            |
| Field of view         | Measured panel width/height visible at that distance, using a ruler in the panel plane                                                        |
| Required regions      | All three character positions and both LEDs fit in the complete preview, with space to tolerate placement variation                           |
| Focus                 | Which strokes/LED edges are sharp or soft; nearest/farthest tested distance that stays readable                                               |
| Exposure and lighting | Panel text/LEDs distinguishable or blown out; room lighting versus shielded view; settling after changes                                      |
| Refresh/flicker       | For at least 30 stationary seconds, note incomplete characters, missing strokes, bands, or changes not seen by eye on the panel               |
| Panel state           | Copy only the actual displayed text/numeric reading and each LED's visible color/on/off/flash behavior; wait for naturally occurring states   |
| Preview health        | Frame number continues advancing; age-at-draw range; any stale/error screen or stalled image                                                  |
| Power and temperature | Resets/flicker and elapsed operating time; ambient/component temperature and voltage/current only if actually measured, with units and method |

Try several measured distances bracketing a useful fit. A single readable view
does not establish a placement tolerance. The optical envelope must name the
tested distance range, orientation, visible margins, focus, lighting/shield,
panel states, and gaps. Camera exposure/gain/white-balance remain at the pinned
driver's startup defaults; no manual override is implemented. Record exposure
appearance and refresh effects without inventing exposure times. A fixed
exposure configuration or additional source-frame inspection may be needed after
these observations. Final calibration and interpretation labels remain separate
work.

References:
[native display component](https://components.espressif.com/components/espressif/esp_lcd_gc9a01/versions/2.0.4/readme),
[pinned GC9A01 source](https://github.com/espressif/esp-bsp/tree/618b05297dae1aa3fab986d5ca7b89b328e1919a/components/lcd/esp_lcd_gc9a01),
[ESP-IDF 5.5.5 LCD lifecycle](https://docs.espressif.com/projects/esp-idf/en/v5.5.5/esp32s3/api-reference/peripherals/lcd/index.html),
and [board wiring](hardware-board-profile.md).
