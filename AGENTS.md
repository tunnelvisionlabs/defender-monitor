# Repository guide

Read [README.md](README.md) and [architecture](docs/architecture.md) before
changing project boundaries. Read [hardware notes](docs/hardware.md) before
board, camera, display, or mount work, and [testing guidance](docs/testing.md)
before changing behavior or tests.

## Scope and implementation

- This is a foundation-stage project. Keep planned behavior clearly labeled;
  scaffolding is not evidence of working capture, recognition, MQTT, or
  hardware.
- Use direct ESP-IDF for the XIAO ESP32S3 Sense. Use the SDK and target in
  `toolchain.json`; pin external components when first introduced and retain
  their dependency lockfile. Do not add moving dependencies or build-time
  downloads of unpinned assets.
- Keep hardware access, image interpretation, transport, and display behavior
  separate. Keep `firmware/main/` for composition as components are introduced.
  Add component boundaries when they have real consumers, not speculative APIs.
- Start Home Assistant support with MQTT discovery. Keep the observation model
  independent of MQTT. Add a custom integration only for a concrete unmet need.
- Observe the Defender control panel without operating it or modifying RO wiring
  or plumbing. Do not invent panel fields, units, states, or dimensions.
- Treat calibration as specific to camera, panel, and mount geometry. Keep
  ambiguous, stale, invalid, and disconnected observations distinct from valid
  readings. Do not present guesses or old data as a fresh measurement.
- Bound buffers, frame ownership, queues, retries, and storage as those features
  are implemented. Record memory and timing targets separately from
  measurements.
- Preserve editable CAD sources. Decide the CAD tool after measurements; keep
  generated print files in ignored `hardware/mount/exports/` during development.

## Privacy and device access

- Keep credentials, private hostnames, device identities, calibration files,
  panel captures, and raw logs in ignored `local/` storage. Public fixtures and
  configuration examples must be fictional. Do not embed secrets in firmware,
  tracked SDK configuration, test artifacts, issues, or PRs.
- Do not identify private reference projects by names, links, or workspace paths
  in tracked files, commit messages, issues, or PRs.
- Routine checks must not load private configuration, access the household
  installation, publish discovery, flash hardware, or provision devices.
- Device writes need an explicitly authorized target. Building firmware does not
  authorize flashing it. Live household checks stay read-only unless the user
  explicitly authorizes the particular change.
- Preserve transport authentication and certificate verification when added;
  test with fictional peers rather than bypassing checks for convenience.

## Verification and documentation

- Add meaningful regression coverage with behavior changes. Exercise production
  objects and public outcomes; use shared, faithful fakes at external
  boundaries. Do not add tests for prose, empty placeholders, or incidental
  implementation details. Introduce test infrastructure with its first consumer.
- Base additional test isolation on observed failures, measured cost, or
  concrete evidence. Do not replace production behavior merely to make a test
  convenient.
- Name tests by scenario and outcome. Explain subtle user or caller risks. Run
  checks appropriate to the change and report actual results and remaining gaps.
- Compile affected firmware before calling it buildable. Separate formatting,
  compilation, host execution, simulated I/O, and physical acceptance. An
  unavailable check is not a pass.
- Document shared API contracts where declared: units, accepted inputs, unknown
  values, ownership/lifetime, task context, blocking, side effects, and failure
  behavior where relevant. Explain constraints rather than repeating types.
- Describe guarantees enforced today; label future contracts as provisional.
  Keep implementation rationale beside non-obvious code. Update affected guides
  with behavior changes without broad comment, test, or formatting rewrites.
- Run `npm run check` for supported documentation/configuration formatting.
  Prettier does not validate C or CMake. Follow the development guide for
  builds.

## Git and writing

- Never close a GitHub issue without explicit user authorization. Completing
  local work, passing checks, or preparing changes does not authorize closure.
  If closure depends on a merge into `main`, keep the issue open until that
  merge occurs.
- Preserve the existing Git index. Do not stage, commit, amend, reset, unstage,
  or perform equivalent index changes without an explicit request for that
  specific action in the current task. Staging and committing require separate
  authorization. Leave completed changes unstaged by default.
- Suggest a Conventional Commits message in a fenced code block after repository
  changes, unless the user requests otherwise. A suggestion does not authorize
  staging or committing. Report modified paths and relevant validation.
- Use an imperative, specific subject of at most 50 characters, a blank line,
  and body lines at most 72 characters. Explain why before what; leave routine
  successful validation in the response. Preserve actual PowerShell newlines,
  and verify any explicitly authorized commit with `git log -1 --format=%B`.
- Use the Oxford comma in prose lists. Follow
  [contribution guidance](docs/contributing.md) for issues, PRs, and messages.
