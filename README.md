[简体中文](README.zh_CN.md) · English

# AI Passport — OpenSwiftUI Embedded display

This fork's `main` branch builds an Embedded display/input profile of OpenSwiftUI for
FoloToy's ESP32-C3. Edit `main/swift/ContentView.swift` to describe the screen:

```swift
import OpenSwiftUI

struct ContentView: View {
    @State private var useBlue = false
    @State private var showImage = true

    var body: some View {
        VStack(spacing: 12) {
            if showImage {
                Image("spark").resizable().frame(width: 80, height: 80)
            }
            Text("Hello, OpenSwiftUI!")
            HStack(spacing: 8) {
                Color.red.frame(width: 32, height: 8)
                Color.blue.frame(width: 32, height: 8)
            }
        }
        .padding(12)
        .background(useBlue ? Color.blue : Color.black)
        .onPhyicButton(.up) { useBlue = false }
        .onPhyicButton(.down) { useBlue = true }
        .onPhyicButton(.ok) { showImage.toggle() }
    }
}
```

The framework is compiled separately into `OpenSwiftUI.swiftmodule` and
`libOpenSwiftUI.a`, then imported and linked by the firmware. The shared View
protocol, builder, empty and conditional views are compiled with an explicit
Embedded configuration; platform-dependent primitives have small Embedded
implementations. This is a **bounded Embedded subset**, not the desktop
AttributeGraph/RenderBox renderer or a full SwiftUI-compatible runtime.

The board starts in ContentView. UP/DOWN cycle the panel palette; OK toggles
the image. These single-click closures update root `@State` and trigger measured
layout and scene replacement. Long OK returns to the hardware menu; reopening
`OpenSwiftUI` creates fresh state. The original counter model/adapter remain as regression
tests. The FoloToy sky/grass/header and boot battery snapshot surround the scene.

## OpenSwiftUI 2048

The sharing build starts directly in 2048. See the [release guide](docs/development/release/pocket-2048.md) for bundles, installation and official publisher capture.


Long-press OK to leave the opening display demo, then select **2048** in the
hardware menu. The original OpenSwiftUI demo remains available.

| Input | Vertical mode (default) | Horizontal mode |
| --- | --- | --- |
| UP | Move up | Move left |
| DOWN | Move down | Move right |
| Short OK | Switch to horizontal | Switch to vertical |
| Long OK | Return to the menu | Return to the menu |

The footer always shows the current mapping. Switching axes preserves the board
and score and does not spawn a tile. UP/DOWN act once on press; short OK acts on
release without waiting for the double-click window. Rapid taps remain separate
actions, and holding a direction does not repeat. Power remains a hardware switch.

Each game starts with two tiles. A changed move adds one tile (90% 2, 10% 4),
and each tile merges at most once per move. Invalid moves preserve the board,
score and random sequence. Reaching 2048 or running out of moves ends the round;
short OK starts a fresh round in vertical mode. Exiting also discards the round.
This version has no persistence, undo or swipe input.

- Rules: `main/swift/PassportCore/Game2048.swift`, shared with SwiftPM tests.
- View: `main/swift/Game2048/Game2048View.swift`, retaining the game in root `@State`.
- Board: four typed `VStack`/`HStack` background rows and a custom `Layout` for
  moving tiles, with 42-pixel tiles, 4-pixel gaps and 4-pixel padding.
  OpenSwiftUI measures and places the 188x188 board without offsets.
- Rendering: `main/openswiftui_scene.c` supplies page-specific `RootGeometry`
  insets (top 46, sides 12, bottom 34) and the common LVGL sink. The score sits
  beside the boot battery snapshot; controls end above the grass.
- Input: `main/demo_2048.c` translates press/release events into the existing
  OpenSwiftUI button closures. Long OK is still handled by the app menu.

The firmware and host preview use a bounded 48 KB LVGL pool for the game
scene (24 KB more static RAM than the original display demo). The 64-bit host
preview checks the pool and reports its peak; device heap measurements remain
a separate acceptance step.

The animated game requires OpenSwiftUI Embedded profile 4 from the
`embed/folotoy` branch; no additional framework repositories are required.
This profile has static text and no dynamic `ForEach`, so finite tile labels and
six score digits use `StaticString`, with rows and foreground slots declared explicitly.
[eleev/swiftui-2048](https://github.com/eleev/swiftui-2048) and
[unixzii/SwiftUI-2048](https://github.com/unixzii/SwiftUI-2048) informed the separation
of the game model, board and tile views. This is a new Embedded implementation;
it does not import their application code, assets, Combine or gesture handling.

## Repository layout and build

Use two independent Git clones: this firmware fork on `main` and OpenSwiftUI
on `embed/folotoy`. Keep OpenSwiftUI as the only source checkout under
`framework/`; its `.git/` directory belongs to this clone. This setup does not
use linked Git worktrees or OpenSwiftUI-Mono workspace scripts.

OpenAttributeGraph (OAG), OpenRenderBox, OpenCoreGraphics, OpenObservation,
Compute, and DarwinPrivateFrameworks are not compiled or linked by this
Embedded profile, so no additional framework repositories are needed.

The directory layout is:

```text
FoloToy/
  ai-passport/                 independent firmware clone, branch main
  framework/
    OpenSwiftUI/               standalone clone, branch embed/folotoy
  toolchains/esp-idf-v5.5.3/
  toolchains/espressif/
  work/                       logs, previews, build outputs and staging
```

For a fresh checkout, clone this firmware fork's `main` branch and OpenSwiftUI's
`embed/folotoy` branch:

```bash
mkdir -p FoloToy/framework
cd FoloToy
git clone --single-branch --branch main https://github.com/OpenSwiftUIProject/ai-passport.git ai-passport
git clone --single-branch --branch embed/folotoy https://github.com/OpenSwiftUIProject/OpenSwiftUI.git framework/OpenSwiftUI
cd ai-passport
```

### Set up OpenSwiftUI for an existing firmware checkout

From the `ai-passport` repository root, clone the framework into the default
location:

```bash
mkdir -p ../framework
git clone --single-branch --branch embed/folotoy \
    https://github.com/OpenSwiftUIProject/OpenSwiftUI.git ../framework/OpenSwiftUI
```

Firmware and host scripts discover `../framework/OpenSwiftUI` automatically;
no environment override is needed with this layout. ESP-IDF creates the framework
module and archive inside its build directory. The LVGL host preview writes its
build output to `../work/openswiftui-preview/`; a standalone framework build
writes to the path supplied with `--output`.

If you already have this OpenSwiftUI branch cloned elsewhere, reuse it and set
its absolute path instead of cloning again:

```bash
export OPENSWIFTUI_SOURCE_DIR=/absolute/path/to/OpenSwiftUI
```

The checkout must contain `Embedded/sources.txt` and `Embedded/idf.cmake`.
The firmware builds and links the framework automatically; no manual archive
copy or OAG setup is required. See the
[OpenSwiftUI Embedded guide](https://github.com/OpenSwiftUIProject/OpenSwiftUI/blob/embed/folotoy/Embedded/README.md)
for standalone host builds and the renderer contract.

### Select the toolchain and build

Install ESP-IDF 5.5.3 and its ESP32-C3 tools using the
[environment guide](docs/development/engineering/environment-setup.md), then
install Swift 6.3.1 RELEASE with Embedded RISC-V libraries. When using existing
installations outside the default sibling paths, select them explicitly:

```bash
export IDF_PATH=/absolute/path/to/esp-idf-v5.5.3
export IDF_TOOLS_PATH=/absolute/path/to/espressif-tools
export SWIFT_TOOLCHAIN=/absolute/path/to/swift-6.3.1-RELEASE.xctoolchain
```

Run from this firmware repository; the initial build resolves the pinned
Managed Components, including LVGL and `espressif/idf_swift`:

```bash
tools/with-env.sh ./tools/test-openswiftui.sh     # verify framework + ContentView integration
tools/with-env.sh ./tools/validate.sh             # host tests + isolated C3 gate
tools/with-env.sh idf.py build                    # incremental development
tools/with-env.sh idf.py size
```

The build automatically rebuilds the separately compiled OpenSwiftUI module
when any selected source changes. `Embedded/sources.txt` in the framework
checkout is the source selection, and `Embedded/idf.cmake` connects it to IDF.
Set `OPENSWIFTUI_SOURCE_DIR` before a fresh build to use another clone; an
existing CMake cache can be changed with `idf.py -DOPENSWIFTUI_SOURCE_DIR=... build`.
The framework's `Embedded/README.md` also documents standalone host builds and
tests that require neither ESP-IDF nor LVGL. Its build script enables
`OPENSWIFTUI_LVGL && hasFeature(Embedded)` through the LVGL flag and the
compiler's Embedded Swift mode.

Toolchain: **ESP-IDF 5.5.3**, **Swift 6.3.1 RELEASE** with RISC-V Embedded
libraries, CMake 3.29+, Ninja and Python 3.12. `tools/with-env.sh` activates only
the current command. It defaults to sibling toolchains and the installed Swift
6.3.1 toolchain; `IDF_PATH`, `IDF_TOOLS_PATH`, and `SWIFT_TOOLCHAIN` override them.
Xcode's native Swift alone does not include this Mac's required RISC-V libraries.
See the [initial toolchain/deployment record](docs/development/engineering/embedded-swift-validation.md).

## Supported display subset

| API | Current behavior |
| --- | --- |
| `struct ContentView: View`, `some View` | Statically specialized body traversal |
| `@ViewBuilder`, nested custom views | Ordered composition, empty blocks, `if`, `if/else` |
| `Color(red:green:blue:opacity:)`, constant colors | Constant sRGB fills; alpha supported |
| `Image("spark")`, `.resizable()` | Intrinsic asset size by default; resizable images accept size proposals with nearest-neighbor scaling |
| `Text("literal")`, `.foregroundStyle(Color)` | Actual Montserrat 14 measurement and width-dependent wrapping; no dynamic localization or CJK font |
| `VStack`, `HStack`, `VStackLayout`, `HStackLayout` | Measure/place layout, explicit spacing, edge/center alignment, and flexible-space distribution |
| `onPhyicButton(.up/.down/.ok) { ... }` | Synchronous platform-dispatched button closures; first matching handler consumes the event |
| `@State`, `EmbeddedViewHost { ContentView() }` | Retained root state; writes invalidate the host and redraw through the existing layout |
| `RootGeometry` | Physical screen size and content insets, root size proposal and centering |
| `Layout` | Custom generic layouts with a typed cache and separate size/placement phases |
| `ZStack`, `.frame`, `.padding`, `.background`, `.offset` | Measured overlays and modifiers; offset changes drawing, not the space reported to a parent |

`PassportContent` obtains the LVGL display size and theme insets from C and
configures `RootGeometry` before layout: the 240x320 panel currently offers
216x224 pixels after top 66, bottom 30 and side 12 insets. Swift does not hardcode
the content size. The renderer proposes that space, measures children, then
places them. Text dimensions come from the same font/wrapping code as the label.
The sample ContentView uses stacks and no offsets.

The Embedded `Layout` contract uses integer pixel geometry and generic,
index-based child access. Its measure/place flow and flexibility probes follow
the framework's layout model; the desktop AttributeGraph `RootGeometry` rule
and full `StackLayout` engine are not linked. Default spacing is a constant
8 pixels. Baseline/RTL alignment, layout priorities, Spacer,
full graph-backed state/observation, desktop animation APIs, SF Symbols and runtime image decoding remain
outside this profile. Unimplemented APIs are unavailable at compile time.

`onPhyicButton` uses the requested spelling. Matching outer modifiers take
precedence; otherwise containers visit children in declaration order and stop
at the first match. Absent branches receive no events. This is a board input
modifier with no focus or hit-testing requirement. The display demo forwards only
CLICK; 2048 forwards UP/DOWN PRESS and a paired short OK RELEASE. DOUBLE never
adds an action, and long OK returns through the menu.

Construct the root inside `EmbeddedViewHost`'s builder and retain that host until
exit. This profile supports `@State` in that retained root and stored children;
new stateful children constructed during `body` evaluation are unsupported and
fail explicitly, rather than silently resetting state. There is no `$state`
binding projection, automatic structural identity, observation or asynchronous
state scheduling. All state/action/render access must use the serialized UI
context. The firmware redraws after an action only when the host is invalidated.

The BSP callback enqueues without waiting. An app-lifetime LVGL timer drains at
most 16 events every 16 ms; overflow drops new events and reports a warning.
Page generations discard queued events from an exited page. Redraw reuses
compatible scene objects in drawing order, keeping the title, battery and content container. Sink
failures retain state for the next action to retry; page exit releases the host.

The C scene sink owns LVGL objects under the existing LVGL lock. It permits at
most 32 drawing nodes for the display demo and 64 for 2048, rejects invalid
coordinates and unknown assets, copies bounded text, and deletes all scene
objects on exit. The display sample uses 8 nodes (7 with the image hidden);
a full animated 2048 board with its fixed cell background and six score digits uses at most 58.
No full-screen framebuffer is added to the MCU. The sample image is 512 bytes
of constant RGB565 Flash data; see [assets](assets/README.md).

### 2048 animations

The game uses `withAnimation(.linear(duration: 0.24))` for tile movement,
followed by a 160 ms scale insertion for merged tiles and scale/fade for newly spawned tiles.
The Swift rule model records both source tiles and their shared merge destination;
result values appear after movement. Nonmerged tiles keep their IDs; merged and
spawned tiles receive new IDs. A custom `Layout` places the 16 foreground slots
at their destination cells. The fixed background and surrounding UI still use
VStack/HStack; no offsets or whole-screen bitmap are used.

This requires OpenSwiftUI Embedded profile **4**, including `withAnimation`,
`.id(UInt32)` and `.transition(.scale.combined(with: .opacity))`. Rebuild the
framework and firmware together. It remains a limited animation subset, without
`.animation(_:value:)`, springs or general desktop Animatable support.

A page-owned LVGL timer advances the host with actual elapsed milliseconds and
requests another frame only while needed. The requested interval is 16 ms;
actual panel frame rate depends on rendering/SPI time and must be measured on
the device. Each phase starts its clock after layout has published the first frame, so layout
work and preceding idle time do not skip the opening frames. The renderer reuses
LVGL objects and updates only changed geometry, styles and label text. The
48 KB LVGL pool and 64-node game limit remain unchanged.

`CONFIG_BSP_LVGL_TASK_STACK_SIZE=12288` budgets 12 KiB for the LVGL task.
The previous 7 KiB default exhausted its stack during real-device 2048 play and
triggered a stack-protection reset, returning to the boot ContentView.

During the two animation stages, an eight-action FIFO preserves the order of
UP/DOWN moves and OK axis changes. Overflow drops the newest action. Per-move diagnostics are disabled in normal
firmware to avoid USB logging stalls.
Long OK exits immediately through the existing page dispatcher, deleting the
timer and dropping pending actions before releasing game state. Timer-allocation
failure falls back to immediate game updates. Invalid moves do not animate or
spawn. Restart is immediate; score and footer always reflect committed rules.
Text keeps the fixed Montserrat font; merged values remain visible while their backgrounds grow, and newly spawned values fade in.


### Optional device stress diagnostic

Build in a separate directory; this flag is **OFF** in normal firmware:

```bash
tools/with-env.sh idf.py -B ../work/2048-soak \
  -D SDKCONFIG="$PWD/../work/2048-soak/sdkconfig" \
  -D SDKCONFIG_DEFAULTS="$PWD/sdkconfig.defaults" \
  -D PASSPORT_2048_SOAK=ON build
```

The diagnostic starts 2048 automatically and submits 1,200 actions through the
production input queue/dispatcher, cycling dense, merge-heavy and seeded boards.
Physical ADC input is disabled in this build. Serial `2048-soak` records stack
headroom, free heap, LVGL pool integrity and completed panel refreshes; the final
`PASS steps=1200` means the run completed. This does not test physical contacts.
Flash only through the device-appropriate verified application workflow, then
restore a normal build with this option OFF. Host PNG/GIF timing is not a measure
of the panel frame rate.

## Preview and tests

```bash
tools/with-env.sh ./tools/test-openswiftui.sh
tools/with-env.sh ./tools/preview-openswiftui.sh ../work/captures/openswiftui-preview.png
```

The preview builds the same pinned LVGL sources on the host and uses the real
C scene sink, actual `ContentView`, framework module and pixel asset. It renders
240x320 RGB565 pixels and saves PNG through the checked screenshot protocol.
Its battery is an explicit **73% host fixture**; it is not a device screenshot.
The preview also checks physical-key filtering, palette/image state, 500 redraws,
unknown assets, node limits and 20 page lifecycles with state reset.
The same command also writes `-2048.png` and `-2048-horizontal.png` previews of a
seeded new game, plus `-2048-tiles.png`, `-2048-won.png` and `-2048-lost.png` from
explicit host fixtures. It also writes six `-2048-animation-0.png` through `-2048-animation-5.png`
frames from an explicit merge fixture. It checks intermediate positions, both
animation stages, object reuse, FIFO saturation, 200 animated moves, exit during
animation, full-board text bounds, 500 game redraws, unchanged-frame redraw
suppression, stale clock inputs and bounded-pool integrity. It also writes 27
`-2048-motion-00.png` through `-2048-motion-26.png` frames at 16 ms host intervals.
It requires `managed_components/lvgl__lvgl` to have been resolved by an IDF build.

Host tests also exercise the actual framework/client module boundary, nested
builders, conditional branches, geometry and colors, plus all eight sink failure
positions in the firmware's real ContentView. These are separate from device
boot, panel output, heap and USB capture checks. Current results are in the
[OpenSwiftUI validation record](docs/development/engineering/openswiftui-embedded-validation.md).

## Device deployment and USB capture

The connected gift previously received app-only test firmware. Its original
partition table has **no Recovery entry**, and the reserved Recovery region was
empty before flashing. The original full Flash backup is retained privately in
`../work/device-backups/`; the last known working screenshot app is staged under
`../work/deployment/screenshot-fd28679a/`.

For this specific unit, extract the application from the verified merged image
and write **only the application at `0x10000`**. Preserve the original bootloader,
partition table, NVS, identity, resources and Recovery region. Recheck the device
and preserved-region digests before and after writing. Never erase the device or
use the merged image as a raw whole-device write for this legacy unit.

The full gate publishes only `build/FoloToy-AI-Passport-full.bin`; incremental
build segments may have different timestamps, so extract app-only deployment
bytes from that verified artifact. Keep the 3 MB application limit and protected
ranges in the [BLE/Recovery contract](docs/development/engineering/ble-recovery-compatibility.md).
No device credentials or private recovery URLs belong in Git.

After a successful deployment, close other serial monitors and capture:

```bash
tools/with-env.sh python tools/capture_screen.py --output ../work/captures/passport.png
```

The tool autodetects a single Espressif USB device, or accepts `--port`.
It follows the no-reset RTS/DTR sequence, checks per-row CRC32 and complete pixel
coverage, and saves only a complete frame. Capture temporarily holds the UI lock
and uses the existing 20-line LVGL buffer. It cannot measure physical brightness
or panel defects. This is an on-demand screenshot facility, not video streaming.

Remote GitHub CI still needs Swift provisioning and the paired framework changes.
Known unused `BUTTON_VER_*` Swift warnings and the smaller Recovery-partition size
warning remain; the verified application belongs in the 3 MB factory partition.
