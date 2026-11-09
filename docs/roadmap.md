# Roadmap

This is a development sequence, not a statement of implemented capabilities.
Each implementation step should deliver reviewable behavior with its relevant
tests and documented evidence. Mechanical and firmware investigations may run
independently where neither depends on an unverified result from the other.

| Step                             | Deliverable                                                                                        | Completion evidence                                                                                     |
| -------------------------------- | -------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------- |
| Foundation                       | Repository boundaries, empty native firmware target, development checks, and working guidance      | Formatting and scaffold compilation; explicit list of unimplemented features                            |
| Panel and assembly investigation | Actual hardware revisions, panel field/state inventory, pin allocation, and optical measurements   | Recorded observations and unresolved questions, with private captures stored locally                    |
| Camera and display bring-up      | Native capture and basic diagnostic presentation on the selected assembly                          | Firmware build, relevant host checks, and authorized physical capture/display trials                    |
| Mount and shroud prototype       | Editable parameterized model with repeatable seating and full external-light exclusion             | Measured fit, field of view, removal/reinstallation, service access, and lighting trials                |
| Calibration and interpretation   | First verified panel fields with validity and freshness semantics                                  | Independently labeled fixtures, failure cases, measured ESP32 resource use, and held-out panel trials   |
| MQTT and Home Assistant          | Discovered Defender device and verified entities through the existing MQTT integration             | Fictional/local broker and HA scenarios for identity, invalid data, outages, restarts, and recovery     |
| Deployment and acceptance        | Provisioning, persistent settings, diagnostics, update/recovery design, and installation procedure | Relevant regression checks plus explicitly authorized installation, thermal, and sustained-run evidence |

Choose the first entity set from the actual control-panel inventory. The project
does not yet promise particular flow, pressure, water-quality, alarm, or runtime
fields, units, update rates, or accuracy.

Mount dimensions, CAD tool, recognition method, MQTT topics/payloads, Home
Assistant compatibility baseline, credential provisioning, and update strategy
remain open decisions. Record a decision with its evidence in the relevant guide
when it is made.

A custom Home Assistant integration is deferred. Revisit it only after naming a
specific user-visible requirement, evaluating the MQTT alternative, and
recording the gap. Do not create a parallel integration simply to mirror MQTT
entities.
