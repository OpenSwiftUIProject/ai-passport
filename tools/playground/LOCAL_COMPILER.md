[简体中文](LOCAL_COMPILER.zh_CN.md) · English

# Your local Swift compiler

The hosted page plays a bundled WASM example immediately. To edit Swift, run a
compiler on your own computer and click **Connect**. The creator's computer does
not need to stay online. No existing repository checkout is assumed.

## Start from scratch

Install Swift 6.3.1 RELEASE (Embedded wasm32 libraries and LLVM tools), Python
3.9+, Git, CMake, Ninja and Node.js 20+ with npm. macOS arm64 is the validated host;
Linux and WSL remain unverified. Use the official [Swift installer](https://www.swift.org/install/).
On macOS, `brew install cmake ninja node python git` supplies the other tools.

```sh
git clone --branch main --single-branch https://github.com/OpenSwiftUIProject/ai-passport.git
cd ai-passport
./tools/playground/start-compiler.sh --allow-origin https://openswiftuiproject.github.io
```

The webpage displays the command using its **actual origin**, computed from
`location.origin`, including a local preview's current port. Allow only that
scheme + host + optional port, without `/ai-passport/`. Repeat `--allow-origin`
for additional trusted sites. All pages on the same origin share this permission.

Alternatively, give the page's downloadable **AI Passport local compiler skill**
to your assistant. It contains its own clone helper and full setup instructions:
[skill source](https://github.com/OpenSwiftUIProject/ai-passport/tree/main/skills/ai-passport-local-compiler).
Both routes clone the public fork's **main** branch; no pinned Passport revision
or Simulator checkout is required.

The launcher prepares pinned OpenSwiftUI and LVGL sources in `build/playground/`,
checks WASI SDK 34 archives in the sibling `toolchains/`, installs locked editor
packages, and caches compiled objects. First setup needs Internet access. It
uses this Passport checkout's source; it does not fetch another Passport copy.
Quick WASM preview needs neither ESP-IDF nor OAG.

Swift defaults to the macOS 6.3.1 release toolchain when installed, then PATH.
Set `SWIFTC=/full/path/to/usr/bin/swiftc` for a different installation of 6.3.1.
Use `--port 4201` if 4191 is occupied, `--prepare-only` to prepare and exit,
or `--cache-dir /another/path` for a separate dependency/object cache.
`OPENSWIFTUI_SOURCE_DIR` and `LVGL_SOURCE_DIR` explicitly select existing developer
sources. The default pinned revisions are listed in [README](README.md).

Keep the terminal running. The Compiler URL defaults to
`http://127.0.0.1:4191/compile`. Click Connect and allow local network access when
the browser asks. A saved URL or `?compiler=...` only prefills the field; it does
not connect automatically. Edit Swift and wait for auto preview, or click
Build & Run. Disconnect preserves the running preview. Tab edits are not saved
to your checkout and a page refresh discards them.

## Build firmware and hand it to Simulator

After a successful preview, **Build firmware** compiles the same ContentView
for the device. This optional step requires ESP-IDF **5.5.3** and the Swift
Embedded RISC-V libraries. From the Passport checkout, for a new installation:

```sh
mkdir -p ../toolchains
export IDF_PATH="$(cd ../toolchains && pwd)/esp-idf-v5.5.3"
export IDF_TOOLS_PATH="$(cd ../toolchains && pwd)/espressif"
git clone --branch v5.5.3 --recursive https://github.com/espressif/esp-idf.git "$IDF_PATH"
"$IDF_PATH/install.sh" esp32c3
./tools/playground/start-compiler.sh --allow-origin https://openswiftuiproject.github.io
```

Reuse a matching existing installation instead of cloning over it. Pass its
`IDF_PATH` and `IDF_TOOLS_PATH` to the launcher. See the repository's
[environment guide](https://github.com/OpenSwiftUIProject/ai-passport/blob/main/docs/development/engineering/environment-setup.md)
for system prerequisites and installation troubleshooting.

The first firmware build takes longer; subsequent builds reuse their own C/Swift
cache. The build log is visible on the page. The server verifies the merged image
against the bootloader, application, partition table and protected-region rules.
**Download full.bin** downloads that checked image, with a verified SHA-256.
It boots the submitted ContentView. A full image is for offset **0x0**; this tool
does not flash devices or provide an application-only download. Preserve device
identity and Recovery when using a separate hardware installation workflow.

To run the image in QEMU, use a Simulator version containing the Playground
import API. In a separate directory/terminal:

```sh
git clone https://github.com/OpenSwiftUIProject/FoloToy-Passport-Simulator.git
cd FoloToy-Passport-Simulator
npm ci
npm start -- --playground-origin https://openswiftuiproject.github.io
```

Use the actual page origin in that command too. The Simulator defaults to
`http://127.0.0.1:4190/`. Enter its URL, click **Send to Simulator**, then **Open
Simulator** to run. The explicit link avoids popup blockers. The image is handed
over as bytes, with SHA-256 validation; nothing is published or uploaded to the
community. The Simulator retains up to three images in memory for ten minutes.
Its cross-origin isolation stays enabled. The local upload capability and an
explicit `--playground-origin` are both required; a default community-only server
rejects this API. The OpenSwiftUIProject fork includes this integration on main;
an older Simulator checkout can still load the downloaded full.bin using its
local firmware file picker.

Editing invalidates download/send controls until the current source has both a
successful preview and matching firmware. Preview state is not transferred:
Simulator boots a fresh instance of the view. Firmware and WASM are different
build targets and runtime behavior can differ.

## Troubleshooting and limits

- Connection refused: check the running terminal and configured port.
- Origin error: `localhost`, `127.0.0.1` and different ports are distinct origins.
- Browser blocks loopback: allow local network access, or use the local editor
  at `http://127.0.0.1:4191/`. Browser and embedded-webview behavior varies; public
  HTTPS Pages-to-loopback compatibility has not yet been validated. Do not disable
  browser security or expose the trusted compiler through a public tunnel.
- Missing ESP-IDF: install it, restart the compiler, and reconnect to refresh capabilities.
- Expired Simulator link: send the firmware again. Rejected import: verify the
  Simulator version, local upload capability and allowed page origin.
- Changed cached sources: the launcher refuses to overwrite them; choose a new
  cache or explicit developer source override.

The compiler binds only to loopback and accepts explicit website origins. It
runs as your user for trusted local development, not public untrusted compilation.
Requests are limited to 64 KiB JSON. WASM builds have a 30-second timeout; one
firmware build can run at a time, with a 20-minute timeout and eight results per
server session. Ctrl-C stops the service and its active firmware build. Results
and logs remain under ignored `build/playground/firmware-jobs/`; remove old session
directories when they are no longer needed. The application firmware cache is
shared by the local service: run only one firmware-building service per checkout.

For static hosting and the API summary, see [deployment](DEPLOYMENT.md).
