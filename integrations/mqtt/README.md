# MQTT and Home Assistant

## Direction

The initial integration will publish observations and Home Assistant discovery
messages from the ESP32 to an MQTT broker. Home Assistant's existing MQTT
integration will expose the Defender as a device. No broker setup, discovery
payload, publisher, topic schema, or Home Assistant custom component is provided
by this scaffold.

Home Assistant documents
[MQTT discovery, device grouping, and availability](https://www.home-assistant.io/integrations/mqtt/).
Use that maintained contract when implementing this adapter. The deployment
needs an owner-managed broker reachable by both the monitor and Home Assistant.

## Design requirements for the first implementation

- Define the entity inventory from verified panel observations. Record units,
  meaning, invalid values, and freshness requirements before choosing entity
  platforms or device/state classes.
- Use stable logical Defender identity and entity unique IDs, independent of
  display names, network addresses, and MQTT client connection IDs. Decide how
  camera replacement and recalibration preserve that identity.
- Group observation entities under the Defender device. Identify the monitor
  hardware/firmware in diagnostics without misrepresenting it as Defender
  controller firmware. Decide whether separate monitor diagnostic entities need
  their own linked device when the concrete entity set is known.
- Keep transport availability separate from valid, fresh panel observations.
  Define connection loss, unreadable frames, invalid calibration, and expired
  observations explicitly, including last-known-value presentation.
- Specify discovery replay, retain policy, birth/last-will behavior, removal,
  and recovery after monitor, broker, and Home Assistant restarts. Test that
  replayed data does not appear to be a new observation.
- Specify payload versioning, timestamps or age semantics, topic ownership, size
  limits, QoS, and reconnect bounds alongside the first implementation.
- Keep credentials private, preserve authentication/certificate checks, and
  avoid broad broker permissions. Normal validation must use fictional/local
  peers and must not publish to the household broker.

These requirements are provisional design inputs, not implemented guarantees.
Exact topics, payloads, discovery style, compatibility baseline, update cadence,
and entity list remain undecided. There is no RO command channel in the planned
monitoring scope.

## Future custom integration

Revisit a custom Home Assistant integration only for a documented user-visible
gap in the MQTT approach. Keep observation semantics portable so that such a
decision does not require rewriting image interpretation. Do not introduce an
empty `custom_components/` package or HACS metadata before an integration
exists.
