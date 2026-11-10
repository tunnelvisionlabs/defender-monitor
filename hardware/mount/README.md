# Mount and light shroud

This directory is reserved for the editable 3D-printable mount model. There is
no CAD geometry or printable part yet. Select the CAD tool and file format after
the physical measurements and optical trials in
[hardware notes](../../docs/hardware.md). Keep editable sources here and
development exports in ignored `exports/`.

## Intended result

Hold the complete XIAO ESP32S3 Sense, camera, and Round Display assembly at a
known location and orientation over the Defender panel. Provide a full shroud
that excludes external light, including seams and cable openings. Tool-free
removal and reinstallation must return the camera to a repeatable position
without requiring recalibration. Geometry changes still require calibration
review.

Keep the assembly non-intrusive and serviceable: retain access to RO controls
and service points through a removable design, avoid permanent RO modifications,
provide cable strain relief, and preserve USB/recovery access. Account for the
display's usability, antenna clearance, heat, moisture, and reflections inside
the shroud. An opaque enclosure alone does not prove stable image quality.

## Current design inputs

The owner requests **USB down** so a shroud around the indicator region can
leave all Defender buttons exposed. Prefer this simpler footprint over adding
button mechanisms; captive manual extensions remain a fallback only if the
measured design must cover a button. Physical fit and full light exclusion still
need verification.

The proposed lens-to-panel target is **39–42 mm**. The owner reports comfortable
full-frame coverage of the three-character display and both LEDs, allowing the
verification display to retain the complete image. This is a working placement
target, not a measured manufacturing/seating tolerance. Focus, shielded-light
stability, numeric field of view, actual cable clearance, and panel mounting
datums remain to be established. See
[preview and optics](../../docs/preview-and-optics.md) for orientation changes,
trial evidence, and remaining checks.

## Measurements before modeling

| Parameter                          | Evidence needed                                                                                  |
| ---------------------------------- | ------------------------------------------------------------------------------------------------ |
| Panel envelope and mounting datums | Actual Defender panel dimensions, surrounding clearances, and permitted attachment surfaces      |
| Observable region                  | Bounds and depth of every required display/indicator region                                      |
| Camera geometry                    | Sensor/lens identity, measured working distance, focus, field of view, and orientation           |
| Board stack                        | Assembled dimensions, connectors, cable bend space, and display access                           |
| Light exclusion                    | Seam overlap, cable entry, panel contact surface, and candidate opaque material                  |
| Attachment repeatability           | Seating/retention method and measured pose changes after removal                                 |
| Thermal and service space          | Measured temperature, ventilation compatible with light exclusion, antenna clearance, and access |
| Printing                           | Chosen material, process tolerances, orientation, fasteners, and fit allowances                  |

Do not substitute guessed dimensions. Record units and coordinate datums with
the model, and relate its revision to compatible camera calibration. Decide
material and sealing details from the actual installation conditions.

## Acceptance when a model exists

Verify fit on the actual panel and assembled boards. Check full region coverage,
focus, reflections, and readings under changed external lighting. Repeat removal
and reinstallation, inspect cable strain and service access, and record
sustained temperature/power behavior inside the completed shroud. Keep private
photos and raw trial logs in `local/`; publish only sanitized measurements and
conclusions.
