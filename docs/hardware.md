# Hardware investigation

## Selected hardware

The owner supplies a US Water Systems Defender whole home RO unit and Home
Assistant. The monitor is planned around the Seeed Studio XIAO ESP32S3 Sense,
its camera expansion board, and the Seeed Studio Round Display for XIAO. Record
the actual unit, control-panel revision, board revisions, and camera sensor
before selecting drivers or drawing mount geometry.

The [board profile and pin allocation](hardware-board-profile.md) records the
current investigation of the supplied assembly. It separates owner-reported
markings, vendor circuit references, and details still requiring measurement or
runtime confirmation.

Seeed documents camera connections separately from the exposed XIAO pins in its
[XIAO ESP32-S3 getting-started guide](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/)
and
[pin-multiplexing guide](https://wiki.seeedstudio.com/xiao_esp32s3_pin_multiplexing/).
Use those references and the matching schematics when building a complete pin
allocation; do not infer ESP-IDF GPIO numbers from Arduino `D` labels.

The Round Display is documented as a 240 × 240 touch display on a 39 mm × 39 mm
board. Its v1.1 schematic adds a KE switch that disconnects battery sensing on
D0 and backlight control on D6; it does not swap those signals between pins.
These are vendor specifications, not measurements of the owner's assembly. See
the [Round Display guide](https://wiki.seeedstudio.com/get_start_round_display/)
and the circuit references in the board profile.

## Combined-board caveat

Seeed's [camera guide](https://wiki.seeedstudio.com/xiao_esp32s3_camera_usage/)
includes a Sense-plus-Round-Display example and describes shared microSD bus and
pull-up interactions. It describes both a Sense-slot software approach and a
Round-Display-slot approach involving a J3 modification. This project has
selected neither approach and does not require microSD storage at this stage.
Confirm the actual board revisions and need for storage before any hardware
modification.

Vendor examples establish a starting point for investigation, not acceptance of
our native ESP-IDF drivers or the assembled monitoring device.

## Evidence to collect before implementation

| Question                                   | Required evidence                                                                                      |
| ------------------------------------------ | ------------------------------------------------------------------------------------------------------ |
| What does the Defender actually show?      | Panel revision, readable regions, labels, units, indicators, and observed operating/transition screens |
| Can the camera read every required region? | Focus and field-of-view trials at measured distances with the complete board stack                     |
| Can both boards operate together?          | Revision-specific camera, display, touch, power, and optional storage pin/bus allocation               |
| What resources are available?              | Actual flash, PSRAM, camera sensor, and usable memory with selected drivers                            |
| Is capture stable?                         | Repeated images covering panel flicker, exposure changes, reflections, and transitions                 |
| Can the assembly run inside the shroud?    | Measured power, temperature, cable routing, antenna clearance, and recovery access                     |
| Is ambient light excluded?                 | Repeatable reading trials under changed room lighting with the final seam and cable openings           |

Keep raw photographs, serial numbers, installation details, and measurement
sessions in ignored `local/`. Summarize non-private results in this guide as
they become available. Do not invent a panel specification or use vendor
dimensions as a substitute for a physical fit check.

The [mount requirements](../hardware/mount/README.md) define the mechanical
work. Firmware compilation does not establish board compatibility, readable
images, thermal behavior, or mount fit.
