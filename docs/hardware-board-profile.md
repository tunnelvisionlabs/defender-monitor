# Board stack and pin allocation

Investigation for
[issue #11](https://github.com/tunnelvisionlabs/defender-monitor/issues/11),
reviewed on 2026-11-09. This is a candidate hardware profile for native ESP-IDF
bring-up. The owner has identified the camera and measured the assembly
envelope; supply behavior and runtime identities remain unverified. No driver,
GPIO configuration, or memory setting is implemented by this document.

## Identified assembly

| Item                       | Evidence                                                                                           | Remaining confirmation                                                           |
| -------------------------- | -------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------- |
| XIAO product               | Owner reports SKU `102010635`; Seeed identifies it as XIAO ESP32-S3 Sense (Pre-Soldered) [product] | Exact base-board revision is not visible                                         |
| Camera                     | Owner reads `OV3660` on the supplied camera                                                        | Read the sensor identity during camera bring-up                                  |
| Connections                | Owner plugged the XIAO into the Round Display; otherwise supplied hardware is unchanged            | Record orientation and service clearances on the actual stack                    |
| Display variant            | Owner identifies a two-section `ON`/`KE` switch, with both sections away from `ON`                 | This matches the reviewed v1.1-style circuit; exact PCB revision remains unknown |
| Power                      | USB power is acceptable; a battery is not required                                                 | Select the actual supply/cable and measure the combined load                     |
| Memory                     | Vendor specification is 8 MB flash and 8 MB PSRAM [product]                                        | Confirm flash capacity, PSRAM operation, and usable memory at bring-up           |
| Complete assembly envelope | Owner measures 43 mm wide, 43 mm high, and 28.7 mm thick; the stack fits behind the screen         | Measurement tool/uncertainty and connector clearances are not recorded           |
| Antenna                    | Owner reports no external antenna connected                                                        | Establish antenna availability, routing, and clearance before Wi-Fi trials       |

The SKU alone does not identify a PCB or camera revision. Seeed's [2025 camera
change notice][camera-change] covers the same SKU, and the current [series
guide][series] describes further camera substitutions. Use the owner's OV3660
marking as the current camera target, with runtime confirmation still required.

The schematic references include base-board v1.51, expansion-board v1.0 and
v1.2, and Round Display v1.0 and v1.1. They are comparison sources, not a claim
that the supplied boards carry those exact revisions. In particular, the
expansion v1.2 drawing changes camera supply components; a shared signal map
does not make the power circuits interchangeable.

## XIAO header allocation

GPIO numbers below are ESP32-S3 numbers. The translation from XIAO labels comes
from Seeed's [pin map][pin-map]; peripheral connections come from the [Round
Display v1.1 schematic][round-v11] and [expansion schematic][sense-exp-v12].

| XIAO label | ESP32-S3 GPIO | Connection on the reviewed stack                   | Initial use                                                      |
| ---------- | ------------- | -------------------------------------------------- | ---------------------------------------------------------------- |
| D0 / A0    | 1             | Display battery-voltage sensing through KE         | Isolated by the reported open switch; no battery sensing planned |
| D1         | 2             | LCD chip select                                    | Display output                                                   |
| D2         | 3             | Round Display microSD chip select                  | Keep card inactive; not a spare output                           |
| D3         | 4             | LCD data/command                                   | Display output                                                   |
| D4 / SDA   | 5             | Display touch and RTC I2C data                     | Leave unused peripherals uninitialized                           |
| D5 / SCL   | 6             | Display touch and RTC I2C clock                    | Leave unused peripherals uninitialized                           |
| D6 / TX    | 43            | LCD backlight control through KE                   | Isolated by the reported open switch; reserve for backlight use  |
| D7 / RX    | 44            | Touch interrupt                                    | Leave as an input; no touch feature planned                      |
| D8 / SCK   | 7             | LCD and both microSD interfaces' SPI clock         | Shared SPI wiring                                                |
| D9 / MISO  | 8             | microSD SPI data to MCU                            | Account for connected card circuitry even when storage is unused |
| D10 / MOSI | 9             | LCD and both microSD interfaces' SPI data from MCU | Shared SPI wiring                                                |

The vendor [display implementation][display-reference] selects a 240 by 240
GC9A01 display, with D1 chip select, D3 data/command, and D6 backlight. This is
controller/wiring evidence for selecting a native driver later, not an Arduino
or LVGL dependency for this project. Its example SPI frequency is not a measured
limit for this assembly.

### KE switch and reset behavior

The [v1.0 display schematic][round-v10] connects battery sensing and backlight
control directly. In [v1.1][round-v11], SW2 disconnects those two paths
independently; both poles away from `ON` leave them disconnected. The separate
SW1 is the power switch. KE does not choose D0 instead of D6 for the backlight.

With the reported KE setting, driving GPIO43 will not control the backlight. The
reviewed circuit biases the disconnected backlight control toward on; that is a
schematic inference, not an observed startup result. Retain the supplied switch
positions for the initial profile and verify visible behavior during display
bring-up. Any later use of the switch must be recorded as a profile change.

LCD reset and touch reset share the display board's power-on reset circuit; the
reviewed schematic does not route them to a separate XIAO GPIO. The vendor
driver likewise uses no GPIO reset. Software reset/startup behavior still needs
physical validation.

## Camera allocation

The base and expansion schematics agree on the following DVP/SCCB signals ([base
v1.51][sense-base], [expansion v1.0][sense-exp-v10], and [expansion
v1.2][sense-exp-v12]). The driver column names the camera data bit, not a XIAO
header label.

| Camera signal | Driver input          | ESP32-S3 GPIO |
| ------------- | --------------------- | ------------- |
| Y2            | Data bit 0            | 15            |
| Y3            | Data bit 1            | 17            |
| Y4            | Data bit 2            | 18            |
| Y5            | Data bit 3            | 16            |
| Y6            | Data bit 4            | 14            |
| Y7            | Data bit 5            | 12            |
| Y8            | Data bit 6            | 11            |
| Y9            | Data bit 7            | 48            |
| XMCLK         | External camera clock | 10            |
| PCLK          | Pixel clock           | 13            |
| VSYNC         | Vertical sync         | 38            |
| HREF          | Line-valid reference  | 47            |
| CAM_SDA       | SCCB data             | 40            |
| CAM_SCL       | SCCB clock            | 39            |

The reviewed expansion circuits hold camera RESET and PWDN through fixed
resistors rather than routing them to MCU GPIOs. Candidate driver values are
therefore no-GPIO reset/power-down (`-1`), subject to matching the supplied
module during bring-up. Do not invent control pins from similarly named signals
on a different ESP32 camera board.

Camera SCCB on GPIO40/39 is electrically separate from display touch/RTC I2C on
GPIO5/6. Select SDK bus ownership when the real drivers are introduced; the
matching protocol does not mean these are one shared physical bus.

## Other connected signals and coexistence

| Function                        | Connection                                                       | Constraint                                                            |
| ------------------------------- | ---------------------------------------------------------------- | --------------------------------------------------------------------- |
| Native USB                      | GPIO19 D- and GPIO20 D+, through the base-board USB-C connector  | Reserve for USB setup, inspection, installation, and recovery         |
| BOOT                            | GPIO0 pushbutton                                                 | Preserve physical recovery access                                     |
| RESET                           | EN / CHIP_PU pushbutton                                          | Not an ordinary GPIO; preserve access                                 |
| User LED / Sense microSD select | GPIO21                                                           | Shared circuit; do not add a heartbeat LED toggle that selects a card |
| PDM microphone                  | GPIO41 data and GPIO42 clock                                     | Connected on Sense; leave unused                                      |
| RTC                             | PCF8563 in the display schematic, on GPIO5/6                     | No RTC driver required for this investigation                         |
| Touch                           | CHSC6X in the vendor reference, on GPIO5/6 with GPIO44 interrupt | No touch driver required for the MVP evidence view                    |

Sources: [base schematic][sense-base], [expansion schematic][sense-exp-v12],
[display schematic][round-v11], and [vendor display
implementation][display-reference].

The camera's reviewed signal set does not overlap the LCD's control/SPI pins.
This supports a candidate allocation, not proof of simultaneous operation. UART0
TX/RX on GPIO43/44 are connected to display functions, so plan to use the
onboard native USB path for computer assistance instead of assuming those header
pins are available for a separate serial cable. Confirm USB startup and recovery
behavior with the pinned SDK during bring-up.

Both microSD circuits remain physically wired to GPIO7/8/9. The expansion
schematic's populated R12 selects GPIO21 for Sense card select; the alternative
D2 connection is unpopulated in that drawing. The display uses GPIO3 for card
select. Keep attached cards deselected before using the shared SPI bus, and
confirm whether either slot contains a card. No microSD driver or storage is
needed for this monitoring baseline.

Seeed's [combined camera/display example][camera-guide] discusses doubled SD
pull-ups and alternatives involving the expansion-board J3/JP3 bridge. Preserve
the unmodified board: no bridge cutting is selected here. Before driving SPI,
reconcile the actual card-select/bridge arrangement and verify capture/display
coexistence rather than copying an example's board modification.

## Power, dimensions, and remaining evidence

The baseline is one USB 5 V source through the XIAO USB-C connector, with
battery inputs unused. It supports operation without a computer. The exact
adapter, cable, and available routing space still need recording. Do not use
isolated board idle-current figures as the supply budget for camera, LCD, Wi-Fi,
and OTA together. The display and base have distinct power circuitry; switch
behavior must not be assumed to disconnect every powered part of a USB-connected
stack.

The supplied complete assembly measures **43 by 43 by 28.7 mm**, as reported by
the owner. Width/height are viewed from the screen face; thickness runs from
that face to the furthest rear component, including the camera/headers. The
measurement method and uncertainty were not specified, so this is an initial
inventory envelope, not a CAD fit tolerance. The owner reports that the stack
fits behind the screen's width/height envelope.

Published sizes are reference envelopes only: the [series guide][series] lists
21 by 17.8 mm for the base and 15 mm height with the Sense expansion; the
[display guide][round-guide] lists 39 by 39 mm for its board. Actual header,
lens, connector, and cable protrusions must be measured before CAD uses them.

| Evidence needed             | How to record it                                                                                                                                  | Completion boundary                                                               |
| --------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------- |
| Complete stack envelope     | Owner reports 43 by 43 by 28.7 mm using the screen-face datum; record method/uncertainty when fit precision is needed                             | Initial envelope recorded; detailed fit measurements belong to mount design       |
| Connectors and access       | USB plug projection/bend space, camera flex clearance, BOOT/RESET access, board orientation, and any header interference                          | Record actual clearances; panel attachment geometry belongs to mount design       |
| Antenna                     | None connected; establish whether the supplied antenna is available, plus its cable route and required clearance                                  | Wi-Fi/OTA readiness gap; camera/display work can remain USB-only                  |
| Switches and cards          | Preserve the reported KE setting; identify the separate power switch and whether either SD slot contains a card                                   | Match the actual stack to the candidate circuit                                   |
| Runtime identity and memory | Sensor ID, flash capacity, PSRAM initialization, and usable memory with drivers                                                                   | Camera bring-up; unknown values do not require new firmware in this investigation |
| Power and temperature       | Record supply/cable, ambient conditions, measurement locations, startup/capture/display load, voltage/brownout behavior, current, and temperature | Measured bring-up results first; Wi-Fi/OTA and enclosed full-load trials later    |

Only the owner's assembly-envelope measurement is recorded. No electrical
measurements, thermal trials, camera captures, or successful boots are claimed.
Keep this investigation open until the remaining antenna, supply/cable, and
service-clearance inventory is recorded. The runtime items above belong to their
implementation steps and must remain explicitly unverified until tested.

## Reference revisions

These are public vendor references inspected for the allocation above. Raw
vendor PDFs and rendered inspection pages are development evidence, not tracked
design files. Owner photographs and raw logs belong in ignored `local/` storage.

- [Pre-soldered Sense product, SKU 102010635][product].
- [XIAO Sense base schematic v1.51, 2026-06-11, electrical sheet 4][sense-base].
- [Sense expansion schematic v1.0, 2023-03-24][sense-exp-v10].
- [Sense expansion schematic titled v1.2, 2026-08-10][sense-exp-v12]. Its
  revision table repeats a v1.1 label; use the title/date to distinguish this
  source, not to infer the owner's PCB revision.
- [Round Display schematic v1.0, 2023-03-08][round-v10], and [v1.1,
  2023-04-07][round-v11].
- [Vendor display implementation][display-reference], inspected file blob
  `616dcdf4410ff0354a0eab07d70419f870e5a4f9`. This is a reference, not a
  selected firmware dependency.

[product]:
  https://www.seeedstudio.com/Seeed-Studio-XIAO-ESP32S3-Sense-Pre-Soldered-p-6335.html
[series]: https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/
[pin-map]: https://wiki.seeedstudio.com/xiao_esp32s3_pin_multiplexing/
[camera-change]:
  https://files.seeedstudio.com/wiki/SeeedStudio-XIAO-ESP32S3/res/PCN-XIAO_ESP32-S3_Sense_Series_Camera_Upgrade.pdf
[sense-base]:
  https://files.seeedstudio.com/wiki/SeeedStudio-XIAO-ESP32S3/new-res/202003753_XIAO%20ESP32S3%20Sense_V1.51_SCH_260611.pdf
[sense-exp-v10]:
  https://files.seeedstudio.com/wiki/SeeedStudio-XIAO-ESP32S3/res/XIAO_ESP32S3_ExpBoard_v1.0_SCH.pdf
[sense-exp-v12]:
  https://files.seeedstudio.com/wiki/SeeedStudio-XIAO-ESP32S3/res/XIAO_ESP32S3_Exp_Board_v1.2_SCH_PDF_260810.pdf
[round-v10]:
  https://files.seeedstudio.com/wiki/round_display_for_xiao/SeeedStudio_Round_Display_for_XIAO_v1.0_SCH_230308.pdf
[round-v11]:
  https://files.seeedstudio.com/wiki/round_display_for_xiao/SeeedStudio_Round_Display_for_XIAO_v1.1_SCH_230407.pdf
[round-guide]: https://wiki.seeedstudio.com/get_start_round_display/
[camera-guide]: https://wiki.seeedstudio.com/xiao_esp32s3_camera_usage/
[display-reference]:
  https://github.com/Seeed-Studio/Seeed_Arduino_RoundDisplay/blob/main/src/lv_xiao_round_screen.h
