# Tests

No feature test suite is implemented yet. See
[testing guidance](../docs/testing.md) for production-boundary coverage,
fictional fixtures, and evidence limits.

Add tests alongside the first behavior they protect. Use host tests for portable
interpretation/model logic, external-boundary fakes for hardware and transport,
and local broker/Home Assistant scenarios for interoperability when needed.
Firmware-specific test projects may live beside the components they exercise;
document their commands here when introduced.

Do not add placeholder passing tests, private panel photographs, real household
identifiers, or automatic live-device access. Keep any future public fixture
corpus independently labeled and fictional or explicitly sanitized, with its
provenance and permitted use documented.
