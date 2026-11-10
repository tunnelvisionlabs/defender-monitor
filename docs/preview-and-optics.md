# Round Display preview and optical measurements

The implemented preview uses native `esp_lcd` with `espressif/esp_lcd_gc9a01`
exactly 2.0.4. The dependency lock also fixes `cmake_utilities` 0.5.3. It adds
no Arduino, LVGL, touch, storage, networking, calibration, or recognition.
Firmware compilation, 49 host scenarios, and a five-minute combined-driver bench
trial pass. The corrected build also passes combined cold-start diagnostics, and
the owner confirms upright, unmirrored UI/camera output and correct colors with
USB pointing up. Panel optical measurements remain pending.

## Preview behavior

The entire 320 by 240 camera frame is sampled into a 192 by 144 preview at
screen coordinates (24, 48). This preserves its 4:3 aspect ratio inside the
240-pixel circular display, including the source corners. It does not reduce the
source frame or select calibrated crops. Small segments that disappear in the
preview might still be present in the source; preview readability cannot
establish recognition accuracy.

The screen has an `UP` marker, `L`/`R` markers, a frame number, and source age
at draw preparation in milliseconds. A bottom strip contains red, green, and
blue in that order, with white `R`, `G`, and `B` labels identifying the intended
hues independently of their positions. The current LCD configuration swaps XY,
uses no additional X/Y mirroring, BGR LCD element order, MSB-first RGB565, and
inversion enabled. Preview sampling reflects source Y; the camera source bytes
and coordinates remain unchanged. The owner confirmed this orientation with the
**USB connector pointing up**, camera lettering upright and unmirrored, and each
labeled bar and known camera-view color correct. This is the chosen screen
reference for this profile, not an inherent top of the circular board. The
complete assembly was rotated; the stock camera/ribbon position is retained.

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

This is the initial build's baseline, before the orientation correction below.
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

### Orientation correction: 2026-11-10

The owner confirmed a moving camera image on the initial build. With the app's
`UP` marker at the top, app-drawn text was reflected left to right, while
camera-view lettering was rotated 90 degrees clockwise without another flip. The
USB connector pointed toward the displayed `L` marker, and the bars appeared
red/green/blue from left to right. These are observations of that initial build,
not a final board-axis definition.

The correction uses the LCD axis swap and a vertical reflection only in the
scaled preview. That preserves the full native 320 by 240 acquisition for later
calibration, with no extra frame buffer or mechanical camera rotation. The
short, semi-flexible stock camera connection remains untouched; the owner is
willing to rotate the whole assembly to choose its final orientation. Lettered
color markers now separate color verification from geometric mirroring.

The revised installed app image SHA-256 is
`ae2ea57d8016981738025004bcbca177fb541872b5fe20791db3c6cfc61f256c`; the startup
ELF prefix is `0db61fc5c069fdea`. It compiles, all 49 host scenarios pass, and
write/hash verification passes. Updated corner tests cover reflected preview Y
and unchanged source bytes. A 20.187-second USB-restart recording contains 83
captures and 82 completed preview transfers at its last report, with zero
reported failures, expirations, replacements, or timeouts. Free heap, PSRAM, and
stack readings match the baseline; the maximum reported copy window is 7,519
microseconds. This short check does not replace visual confirmation or establish
a new five-minute measurement.

The logger then observed USB disconnection/reconnection, with USB unavailable
for 8.203 seconds, and recorded the SDK's power-on reset reason `1` without
requesting a software reset. The same revised build passed PSRAM and
camera/display startup, produced a different boot-session token, and resumed
both timestamp series. The 20.016-second cold-start recording has 83 captures
and 82 completed preview transfers at its final periodic report (15.750
seconds), with zero reported errors, expirations, replacements, or timeouts and
unchanged memory/stack readings. This establishes combined cold-start
diagnostics. The owner confirmed upright, unmirrored camera lettering with USB
pointing up, but the labeled `R` bar appeared blue and `B` appeared red.

The final color correction selects BGR in the LCD controller; canvas and native
camera bytes remain MSB-first RGB565. The installed image SHA-256 is
`e136a05e8e695dbf92ba39ded193975188d024e2bcdce8cfc0c09d77dbb60ead`; its startup
ELF prefix is `ba23833311677c5c`. Firmware compilation and write/hash
verification pass. A second physical power cycle, with USB unavailable for 7.593
seconds, again reports power-on reset reason `1`, successful PSRAM and
camera/display initialization, and advancing source timestamps. Its
20.063-second recording contains 83 captures/submissions and 82 completed
previews at the final periodic report (15.735 seconds), with zero reported
errors, expirations, replacements, or timeouts. Heap/stack measurements and the
7,519-microsecond maximum copy window match the preceding revision.

The owner confirms that the `R`/`G`/`B` labels now match their bar colors, USB
still points up, and known colors also match in the camera view. The upright,
unmirrored camera orientation is retained. No camera repositioning, GPIO change,
source-frame transformation, or extra image buffer was required.

The optical envelope at the Defender remains open. The worksheet below supports
panel work on USB power away from this computer. The source resolution and
exposure defaults remain provisional until that work supplies measurements.

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
