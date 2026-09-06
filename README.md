[简体中文](README.zh_CN.md) · English

# AI Passport — OpenSwiftUI Embedded display

The `embed/folotoy` branch builds an Embedded display/input profile of OpenSwiftUI for
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

## Workspace and build

Only two source repositories are required: this firmware branch and the
OpenSwiftUI `embed/folotoy` branch. OpenAttributeGraph (OAG), OpenRenderBox,
OpenCoreGraphics, OpenObservation, Compute, and DarwinPrivateFrameworks are not
compiled or linked by this Embedded profile. OpenSwiftUI-Mono is optional;
no framework dependency-cloning script is needed.

The minimal sibling layout is:

```text
FoloToy/
  ai-passport/                 firmware, branch embed/folotoy
  framework/
    OpenSwiftUI/               standalone clone, branch embed/folotoy
    build/riscv32/             standalone framework output
  toolchains/esp-idf-v5.5.3/
  toolchains/espressif/
  work/                       ignored-by-location logs, previews and staging
```

For a fresh checkout, use your firmware repository or fork URL for
`<ai-passport-repository-url>`. OpenSwiftUI's branch is available in its upstream
repository:

```bash
mkdir -p FoloToy/framework
cd FoloToy
git clone --single-branch --branch embed/folotoy <ai-passport-repository-url> ai-passport
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

If you already have this OpenSwiftUI branch checked out, reuse it and set its
absolute path instead of cloning again:

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
worktree is the source selection, and `Embedded/idf.cmake` connects it to IDF.
Set `OPENSWIFTUI_SOURCE_DIR` before a fresh build to use another worktree; an
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
| `onPhyicButton(.up/.down/.ok) { ... }` | Synchronous single-click closures; first matching handler consumes the event |
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
full graph-backed state/observation, animation, SF Symbols and runtime image decoding remain
outside this profile. Unimplemented APIs are unavailable at compile time.

`onPhyicButton` uses the requested spelling. Matching outer modifiers take
precedence; otherwise containers visit children in declaration order and stop
at the first match. Absent branches receive no events. This is a board input
modifier with no focus or hit-testing requirement. PRESS, DOUBLE and LONG do
not invoke the page's single-click closures.

Construct the root inside `EmbeddedViewHost`'s builder and retain that host until
exit. This profile supports `@State` in that retained root and stored children;
new stateful children constructed during `body` evaluation are unsupported and
fail explicitly, rather than silently resetting state. There is no `$state`
binding projection, automatic structural identity, observation or asynchronous
state scheduling. All state/action/render access must use the serialized UI
context. The firmware redraws after an action only when the host is invalidated.

The BSP callback enqueues without waiting. An app-lifetime LVGL timer drains at
most 16 events every 16 ms; overflow drops new events and reports a warning.
Page generations discard queued events from an exited page. Redraw replaces
only scene children, keeping the title, battery and content container. Sink
failures retain state for the next action to retry; page exit releases the host.

The C scene sink owns LVGL objects under the existing LVGL lock. It permits at
most 32 drawing nodes, rejects invalid coordinates and unknown assets, copies
bounded text, and deletes all scene objects on exit. The sample uses 8 nodes, or 7 with the image hidden.
No full-screen framebuffer is added to the MCU. The sample image is 512 bytes
of constant RGB565 Flash data; see [assets](assets/README.md).

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
