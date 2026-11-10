# Tests

The camera host suite compiles the production `monitor_camera.c` with the pinned
camera/JPEG public headers and fakes for SDK camera, GPIO, clock, random, and
PSRAM boundaries. It exercises frame ownership, new unchanged scenes, cached or
malformed input, initialization failures, timeout, and historical progress. It
does not emulate the sensor, DMA, real timeouts, or physical image quality.

First configure/build firmware with the pinned SDK so its lockfile resolves the
managed headers. Then use CMake and a host C11 compiler (GCC or Clang):

```powershell
cmake -S tests/host -B .cache/host-tests -G Ninja
cmake --build .cache/host-tests
ctest --test-dir .cache/host-tests --output-on-failure
```

Windows execution was checked with the official Zig 0.15.2 distribution's
`zig cc` (Clang 20.1.2). If using that compiler, set its path explicitly on the
first configuration:

```powershell
cmake -S tests/host -B .cache/host-tests -G Ninja `
  -DCMAKE_C_COMPILER="C:/tools/zig-0.15.2/zig.exe" `
  -DCMAKE_C_COMPILER_ARG1=cc
```

The path is an example. No compiler is installed or downloaded by tests. The
suite uses the actual camera public types instead of maintaining a duplicate
camera API. Each CTest scenario is a fresh process, matching the component's
once-per-boot initialization contract. Fakes enforce SDK buffer borrowing and
invalidate returned pixels; they do not replace capture validation logic.

See [testing guidance](../docs/testing.md) for coverage policy and
[camera capture](../docs/camera-capture.md) for physical acceptance.

Add tests alongside the first behavior they protect. Use host tests for portable
interpretation/model logic, external-boundary fakes for hardware and transport,
and local broker/Home Assistant scenarios for interoperability when needed.
Firmware-specific test projects may live beside the components they exercise;
document their commands here when introduced.

Do not add placeholder passing tests, private panel photographs, real household
identifiers, or automatic live-device access. Keep any future public fixture
corpus independently labeled and fictional or explicitly sanitized, with its
provenance and permitted use documented.
