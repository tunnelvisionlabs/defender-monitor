# Testing and evidence

Tests should specify observable behavior using production objects. A future
maintainer should understand the scenario, expected outcome, and consequence of
failure without reconstructing private implementation details.

## Current checks

`npm run check` verifies supported file formatting. A fresh
`idf.py -C firmware build` compiles and links the camera application; existing
scaffold configurations need the refresh described in
[development](development.md). The [host camera suite](../tests/README.md)
executes production capture code with SDK-boundary fakes and the pinned driver's
real public headers. Preview tests execute production scaling, mailbox,
composition, and freshness behavior against a strict LCD-transfer fake. CI
defines compilation and host execution separately. None establishes physical
capture, image quality, or recognition accuracy. There is no panel corpus,
live-device harness, or recognition benchmark yet.

Documentation-only work needs review and applicable formatting/link checks,
without tests that assert prose or placeholder file existence. Add test
infrastructure when the first meaningful behavior needs it.

## Behavior to protect as features arrive

- Recognition produces a correct observation for verified, calibrated panel
  regions, and rejects ambiguous, occluded, transitional, or invalid input.
- Camera/mount changes invalidate incompatible calibration. Image ownership and
  bounded memory prevent stale buffers from becoming fresh observations.
- Freshness expires predictably. A retained value, connected broker, or running
  ESP32 does not conceal a failed or unreadable camera.
- MQTT discovery creates the intended Home Assistant device and entities with
  stable identity; reconnects and restarts do not create duplicates or report
  stale readings as current. Test broker and HA restart cases independently.
- Invalid settings, authentication failures, full storage, and interrupted work
  have documented outcomes. Credentials are not emitted in diagnostics.

These are future acceptance concerns, not an existing API or payload contract.
Agree field-specific examples and thresholds when the related feature is built.

## Production paths and external boundaries

Exercise public results, observations, MQTT messages, Home Assistant states,
availability transitions, and persistent effects. Avoid private helpers,
incidental call counts, and exact internal ordering unless they are an explicit
resource, ownership, or protocol contract.

Prefer real recognition logic and real file behavior with temporary storage. Use
shared, faithful fakes at camera/SDK, clock, transport, and other true external
boundaries. Add local broker/HA tests when interoperability first needs them;
use real Home Assistant fixtures for its registries and entity lifecycle if a
custom integration is introduced. Keep external network access and private
configuration out of normal test runs.

Additional isolation needs evidence of a real failure, measured cost, or an
uncontrolled dependency. Test-specific overrides should be essential to the
scenario, not a way to replace production behavior for convenience. Keep timing
relationships intact when using controlled clocks. Name tests by scenario and
outcome, and explain subtle regressions' user or caller impact.

Keep public fixtures fictional. Use independently labeled observations for image
tests; do not derive expected values with the same algorithm being tested. If
recognition uses training, separate training, tuning, and held-out evaluation
captures so reported accuracy does not reuse the training evidence. Private
captures remain opt-in local evidence and must not become CI prerequisites.

## Report what a check establishes

| Evidence                         | Establishes                                                    | Does not establish                                           |
| -------------------------------- | -------------------------------------------------------------- | ------------------------------------------------------------ |
| Formatting                       | Supported files match the configured style                     | Runtime correctness or C/CMake validity                      |
| Firmware compilation             | Sources link for the selected SDK and target                   | Boot, wiring, image quality, or board compatibility          |
| Host tests with fictional inputs | Covered production behavior for those inputs                   | Accuracy on the installed panel or physical recovery         |
| Local broker/HA tests            | Covered discovery/state/lifecycle behavior for tested versions | Household deployment or radio reliability                    |
| Authorized device trials         | Behavior observed on the recorded assembly                     | Unmeasured environments, revisions, or long-term reliability |
| Mount and shroud trials          | Measured fit, repeatability, and light exclusion               | Untested material life, temperature, or service access       |

Record actual tool versions, scenarios, outcomes, and remaining gaps.
Distinguish simulated input, compilation, host execution, and hardware
acceptance. Add tests with each meaningful implementation or fix, rather than
deferring all coverage to a final testing phase. An unavailable or blocked check
remains unverified.
