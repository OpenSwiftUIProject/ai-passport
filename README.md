[简体中文](README.zh_CN.md) · English

# AI Passport — OpenSwiftUI Embedded display

The `embed/folotoy` branch builds a static-display profile of OpenSwiftUI for
FoloToy's ESP32-C3. Edit `main/swift/ContentView.swift` to describe the screen:

```swift
import OpenSwiftUI

struct ContentView: View {
    var body: some View {
        VStack(spacing: 12) {
            Image("spark")
                .resizable()
                .frame(width: 80, height: 80)
            Text("Hello, OpenSwiftUI!")
            HStack(spacing: 8) {
                Color.red.frame(width: 32, height: 8)
                Color.blue.frame(width: 32, height: 8)
            }
        }
        .padding(12)
        .background(Color(white: 0.1))
    }
}
```

The framework is compiled separately into `OpenSwiftUI.swiftmodule` and
`libOpenSwiftUI.a`, then imported and linked by the firmware. The shared View
protocol, builder, empty and conditional views are compiled with an explicit
Embedded configuration; platform-dependent primitives have small Embedded
implementations. This is a **static display subset**, not the desktop
AttributeGraph/RenderBox renderer or a full SwiftUI-compatible runtime.

The board starts directly in the View display. This page adds no input behavior.
The inherited long-OK action returns to the hardware menu, whose `OpenSwiftUI`
entry opens it again. The original counter model/adapter remain as regression
tests. The FoloToy sky/grass/header and boot battery snapshot surround the scene.

## Workspace and build

The expected sibling layout is:

```text
FoloToy/
  ai-passport/                 firmware, branch embed/folotoy
  framework/                   branch workspace container
    OpenSwiftUI/               framework, branch embed/folotoy
    OpenAttributeGraph/        sibling source worktrees, currently not linked
    ...
    build/riscv32/             standalone framework output
  toolchains/esp-idf-v5.5.3/
  toolchains/espressif/
  work/                       ignored-by-location logs, previews and staging
```

The workspace was created with OpenSwiftUI-Mono's helper:

```bash
Scripts/setup.sh --worktree Repos embed/folotoy /absolute/path/FoloToy/framework
```

Run from this firmware repository:

```bash
tools/with-env.sh ./tools/validate.sh             # host tests + isolated C3 gate
tools/with-env.sh idf.py build                    # incremental development
tools/with-env.sh idf.py size
```

The build automatically rebuilds the separately compiled OpenSwiftUI module
when any selected source changes. `Embedded/sources.txt` in the framework
worktree is the source selection, and `Embedded/idf.cmake` connects it to IDF.
Set `OPENSWIFTUI_SOURCE_DIR` before a fresh build to use another worktree; an
existing CMake cache can be changed with `idf.py -DOPENSWIFTUI_SOURCE_DIR=... build`.

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
8 pixels. Baseline/RTL alignment, layout priorities, Spacer, dynamic layout,
state/observation, animation, SF Symbols and runtime image decoding remain
outside this profile. Unimplemented APIs are unavailable at compile time.

The C scene sink owns LVGL objects under the existing LVGL lock. It permits at
most 32 drawing nodes, rejects invalid coordinates and unknown assets, copies
bounded text, and deletes all scene objects on exit. The sample uses 8 nodes.
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
The preview additionally checks unknown assets, node limits and 20 page lifecycles.
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
