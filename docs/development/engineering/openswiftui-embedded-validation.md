[简体中文](openswiftui-embedded-validation.zh_CN.md) · English

# OpenSwiftUI Embedded display validation

Recorded 2026-09-06 on `embed/folotoy`. Firmware baseline: `85382aa`;
OpenSwiftUI baseline: `01cb28f6`. These are local prototype changes.

## Profile 3: physical buttons and retained root State (current)

The current ContentView declares root `@State` values for its palette and image
visibility. `.onPhyicButton(.up/.down/.ok)` closures change those values; a
retained `EmbeddedViewHost` invalidates and reruns RootGeometry/layout before
replacing scene children. UP/DOWN cycle the palette, OK toggles the image and
long OK keeps its global menu-return behavior. Reentry resets state.

The app dispatches the BSP events through a bounded, nonblocking queue to a
16 ms LVGL timer. Page generations reject stale queued input after navigation.
The normal OpenSwiftUI State/GraphHost implementation is not linked; this
profile supports State constructed in retained root/stored-child values only.
Stateful children newly constructed in `body`, Binding projection and general
dynamic view identity remain unsupported. No other framework repo was changed.

| Check | Result |
| --- | --- |
| Build | PASS: full isolated ESP32-C3 gate and merged-image contract |
| Host tests | PASS: framework input/layout/rendering, actual Swift/C ContentView, queue ordering/overflow/startup/page epochs and allocation failures |
| LVGL preview | PASS: 500 redraws, single-click filtering, three key actions, stable scene shell, state reset over 20 lifecycles; initial/down/hidden frames each cover 76,800 pixels |
| RISC-V framework | PASS: 35 selected sources, 73,838-byte intermediate archive |
| Application | PASS: 1,557,232 bytes (+7,184 from profile 2), below the 3 MB limit |
| Normal profile | Syntax parse PASS; full desktop build NOT RUN |
| Device tests | PASS: app-only deployment/digest, protected ranges, profile 3 startup and three CRC-checked USB screenshots; physical-key actions not tested |
| Unverified | ADC timing/debounce, physical key response, long-OK navigation, peak heap during interaction, LVGL task stack margin and repeated interactive use |

The C atomic operations use GCC/Clang builtins because the Swift component's
C++ include paths shadow the C `stdatomic.h` header. The final complete gate
passed with the same implementation on the host and ESP32-C3.

Logs: `FoloToy/work/logs/openswiftui-input-full-validation-final.log`,
`openswiftui-input-preview.log`, and `openswiftui-input-default-parse.log`.
The previews use a 73% fixture battery and are host renders, not device captures.
Staging: `FoloToy/work/deployment/openswiftui-input-088515ef/` (uncommitted source hashes in its manifest).
App SHA-256: `5dc10e17859c2383e347ad1f21a30afa10a71bac78b25fbf011e54c65f739410`.
Merged SHA-256: `088515eff9d83d21a9e6e57f483339d1d6ae869b2e99ffc61bb9229516588636`.
Only the staged app may be written at `0x10000` to the legacy unit after its
protected ranges are rechecked; the merged file is not its deployment payload.

Profile 3 was deployed on 2026-09-06. Before writing, the current 3 MB app
partition was backed up with a verified digest; its prefix (including current
NVS) was privately snapshotted. Only 1,557,232 application
bytes at `0x10000` were written. The application digest and both protected ranges
matched after writing. Startup rendered 8 nodes and initialized the three-button
driver; scene free heap was 158,488 bytes. No panic
or watchdog was seen during the 35-second boot observation.

Three no-reset USB captures each verified all 76,800 pixels. Their free heap was
150,428 bytes; screenshot-worker stack margin
was 4,300 bytes (not the
LVGL task's stack). Private deployment/capture reports and the rollback snapshot
are archived in `FoloToy/work/device-backups/20260906T044501Z/openswiftui-input-088515ef/`.
The physical buttons have not yet been exercised for this profile.

## Profile 2: measured layout (historical)

The profile 2 ContentView uses VStack/HStack, padding and backgrounds, with no
ZStack or offset. The platform supplies its actual LVGL screen size and theme
insets before rendering: RootGeometry turns 240x320 into a 216x224 proposal,
measures the root and centers its fitted bounds. Text is measured using the
same Montserrat 14 font and wrapping as the rendered label. Images have intrinsic
size and an explicit resizable policy.

The Embedded Layout protocol executes typed-cache, sizeThatFits and
placeSubviews phases, including custom client layouts. Stack allocation measures
minimum/maximum flexibility, allocates less-flexible children first, then places
children in declaration order. This uses the layout model inspected in upstream
RootGeometry/StackLayout; it does not link their AttributeGraph implementations.
Integer geometry, generic indexed children and fixed default spacing are profile
specific. See the README for unsupported layout semantics.

| Current check | Result |
| --- | --- |
| Cross-module rendering/layout tests | PASS: root insets, changed proposals, stack alignment, flexibility, pixel remainders, overflow, absent children, wrapping, intrinsic/resizable images, modifiers, client-defined Layout |
| Real ContentView C boundary | PASS: 8 draws, all drawing failures, begin/geometry/measurement rejection |
| Real LVGL host preview | PASS: 76,800 pixels, real-font wrap/label agreement, changed display size, resource/node limits and 20 lifecycles |
| Full gate | PASS: host tests, isolated C3 build and merged-image contract |
| RISC-V framework | PASS: 32 selected sources, 65,182-byte intermediate archive |
| Application | PASS: 1,550,048 bytes, +15,040 over profile 1; below the 3 MB limit |
| Normal source profile | Syntax parse PASS with availability macros; full desktop build NOT RUN |
| Device tests | NOT RUN: no Passport USB port present after this build; profile 1 remains the last verified device deployment |

Artifacts are staged in `FoloToy/work/deployment/openswiftui-layout-68e2ba78/`.
The naming correction also passed `openswiftui-naming-full-validation.log`,
using an exact temporary export of OpenSwiftUI `e86ebf2d`.
Only the app at `0x10000` may be written to the backed-up legacy unit after
rechecking its protected regions. Current app SHA-256:
`19d7c0a3d694d28408b875a9dcb660fac361f4e6466bb91f9596470bf961589c`.
Merged SHA-256: `68e2ba784902a0a7fa7807d385766ebd4a8046206d731821787dc058044df604`.
The archive size is not retained Flash usage. Cache arrays are temporary;
MCU stack margin, peak heap and repeated page entry remain unverified for v2.

Logs: `FoloToy/work/logs/openswiftui-layout-host.log`,
`openswiftui-layout-preview.log`, `openswiftui-layout-full-validation.log`, and
`openswiftui-layout-default-parse.log`. Host preview:
`FoloToy/work/openswiftui-preview/layout-host.png` (fixture battery 73%).

## Profile 1: original static display (historical)

The following results and hashes describe the previously deployed profile 1.

| Check | Result |
| --- | --- |
| Workspace | PASS: seven managed framework worktrees, all on `embed/folotoy` |
| Original Color compile probe | Expected failure: Foundation is unavailable for `riscv32-none-none-eabi` |
| Embedded framework | PASS: 19 selected files, separate module and RISC-V archive (28,770 bytes) |
| Generic composition host tests | PASS: nested ContentView, conditional/optional branches, draw order, frames, offsets and color bounds |
| Firmware Swift/C host tests | PASS: actual ContentView and framework module; 8 ordered draws, every sink failure position and begin rejection |
| Headless LVGL rendering | PASS: real C scene, theme, asset and LVGL 9.5; 76,800 checked pixels, asset/node-limit rejection, 20 lifecycles |
| Incremental ESP32-C3 firmware | PASS: 1,535,008-byte application, 3,536 bytes above the previous screenshot application |
| Link map | PASS: `libOpenSwiftUI.a(OpenSwiftUI.o)` is loaded and supplies `openswiftui_embedded_version`; Swift Color symbols are present |
| Complete gate | PASS: repository/workflow checks, all host tests, isolated firmware build and merged-image contract |
| Physical deployment / device-rendered frame / USB capture | PASS: app-only flash, boot render, preserved-range checks and 3 complete USB captures |
| Remote CI / other OpenSwiftUI APIs | NOT RUN |

The 28,770-byte archive is an intermediate object archive, not the amount of
Flash retained after linking. Embedded Swift specializes generic framework
code into the client and the final link discards unused sections. The app-size
delta also includes the changed page and pixel asset; it is not an isolated
framework benchmark.

The host preview uses the actual firmware C/LVGL renderer and a native Embedded
Swift module. Its 73% battery value is a fixture, and its `free_heap=0` log is
an explicit host stub. Neither value is a measurement of the device. Rendering
has been inspected for orientation, RGB swatches, image scaling and text.

## Verified artifact

The isolated gate produced a **1,600,544-byte** merged artifact and a
**1,535,008-byte** application. Only the application is staged for the legacy
unit's app-only write; deployment and device digest verification passed.

```text
Merged SHA-256: ab222746b7fffb2d053df0fa17b10b592d90ce14d8af7018950e240d737e33e8
Application SHA-256: b576edbf9b05f5c52a70ea32d5c1b910e405c8dd29c4a667a9d1e81955552783
Staging: FoloToy/work/deployment/openswiftui-ab222746/
```

The stage contains both artifacts and a manifest with source hashes. The
firmware and OpenSwiftUI changes are uncommitted. Normal-profile source syntax
also passed parsing; the full desktop framework build was not run.

## Device deployment on 2026-09-06

Only the 1,535,008-byte application at `0x10000` was written. The connected
ESP32-C3 has 8 MB Flash, with Secure Boot and Flash Encryption disabled.
The original partition table, bootloader, identity, resources and empty Recovery
area remain installed. Device application digest verification passed.

The preflight check found 2,069 pre-existing changed bytes in NVS sectors
`0xa000` and `0xb000`, compared with the initial full backup. The initial attempt
stopped before writing. Current NVS was privately snapshotted; both protected
ranges (`0x0..0xffff` and `0x310000..0x7fffff`) then matched this pre-deployment
snapshot before and after the application write. All bytes outside NVS still
matched the initial backup. No original NVS was restored over current settings.

The boot log reports `profile=1 render=OK nodes=8 free_heap=159796`, with no
panic or watchdog marker in a 35-second observation. Three USB capture/reconnect
cycles each verified 76,800 pixels, in 0.539, 0.542 and 0.539 seconds. All three
frames were identical, with 151,736 bytes free heap and 4,296 bytes capture-task
stack margin; reconnecting produced no reset marker. The actual device-rendered
PNG was inspected for the image, text, RGB swatches, orientation and layout.
Its 100% boot battery label is a device snapshot, not a battery calibration test.

Private evidence is outside Git in
`FoloToy/work/device-backups/20260906T044501Z/openswiftui-ab222746/`:
pre-deployment protected-region snapshots, flasher/boot/capture logs, sanitized
JSON summaries, and `device-screen.png`. This verifies device rendering and USB
capture; physical panel brightness, long-duration operation, input behavior and
the full desktop framework build were not checked in this deployment.

## Profile boundary

The framework source selection compiles the existing View/PrimitiveView,
ViewBuilder, EmptyView, Never and conditional-content declarations with the
`OPENSWIFTUI_LVGL && hasFeature(Embedded)` condition. The framework build enables
both the LVGL flag and Embedded Swift mode on the host and ESP32-C3.
The graph requirements, MainActor and
runtime tuple metadata are excluded. Ordered generic pairs carry builder
children. Small Embedded implementations provide Color, Image, Text, ZStack,
fixed pixel frames and offsets. Rendering is synchronous through a generic
platform sink; no existential view tree or dynamic graph is created.

This is a new experimental Embedded profile in the OpenSwiftUI worktree. The
normal framework dependency graph has not been ported to the MCU. Foundation,
OpenAttributeGraph, OpenRenderBox, observation, dynamic state, diffing, animation,
SF Symbols, accessibility and full SwiftUI layout semantics are not included.
All eight modified upstream source files retain their normal implementation
behind conditional compilation. Other sibling worktrees are unmodified.

## Reproduce

From the firmware checkout:

```bash
tools/with-env.sh ./tools/validate.sh
tools/with-env.sh ./tools/preview-openswiftui.sh ../work/captures/openswiftui-preview.png
```

The framework's `Scripts/test_embedded.sh` checks generic rendering across a
real module boundary. `tools/test-openswiftui.sh` additionally compiles the actual
firmware ContentView and C boundary fixture. The headless preview compiles the
real LVGL scene rather than replacing its drawing methods with stubs.

Build logs are outside Git under `FoloToy/work/logs/`:
`openswiftui-embedded-host.log`, `openswiftui-embedded-riscv.log`,
`openswiftui-firmware-build.log`, `passport-contentview-interop.log`, and
`openswiftui-lvgl-preview.log`, `openswiftui-full-validation.log`, and
`openswiftui-default-parse.log`.

The device deployment constraints and exact supported APIs are documented in
the [current README](../../../README.md). The prior device backup and preserved
partition checks are recorded in the [initial validation record](embedded-swift-validation.md).

References: [Embedded Swift linkage model](https://forums.swift.org/t/embedded-swift-linkage-model/81441),
[Embedded Swift ABI](https://docs.swift.org/latest/documentation/embeddedswift/abi/).
