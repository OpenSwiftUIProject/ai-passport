---
name: ai-passport-local-compiler
description: Set up a visitor's local Swift compiler from scratch and connect the AI Passport OpenSwiftUI web playground, including cloning the public fork and preparing host dependencies.
---
[简体中文](SKILL.zh_CN.md) · English

# AI Passport local Swift compiler

Set up the compiler on the visitor's computer and connect the browser playground.
This skill works when downloaded by itself: do not assume any project checkout,
Simulator repository, Swift toolchain, or sibling workspace already exists.

- Project: https://github.com/OpenSwiftUIProject/ai-passport
- Clone URL: `https://github.com/OpenSwiftUIProject/ai-passport.git`
- Project branch: **main**, not a pinned Passport revision.
- Intended Pages entry: https://openswiftuiproject.github.io/ai-passport/
- Default compiler URL: `http://127.0.0.1:4191/compile`.

## Prepare the computer

Detect OS/architecture and installed tools first. The validated host is macOS
arm64. The compiler needs **Swift 6.3.1 RELEASE**, including Embedded wasm32
libraries, clang, clang++, wasm-ld, llvm-ar and llvm-ranlib; also Python 3.9+,
Git, CMake, Ninja, Node.js 20+ and npm. Firmware tools and ESP-IDF are not needed for quick WASM preview.

On macOS, reuse Xcode Command Line Tools and Homebrew if installed. Install missing
build tools with `brew install cmake ninja node python git`. If Xcode tools or
Homebrew are absent, use `xcode-select --install` and the official installer at
https://brew.sh as appropriate. Install Swift 6.3.1 from the official download
page https://www.swift.org/install/ (or `swiftly install 6.3.1` when Swiftly is
already installed). Keep existing Swift versions; do not change shell startup
files or the global toolchain selection. If an installer requires an interactive
administrator step, let the user complete it and continue afterward.

Check `swiftc --version` for the selected toolchain. The launcher recognizes
`~/Library/Developer/Toolchains/swift-6.3.1-RELEASE.xctoolchain/usr/bin/swiftc`;
otherwise set `SWIFTC` to the full compiler path for this command. Linux and
Windows/WSL are not yet validated; distinguish an experimental attempt from the
verified macOS setup instead of promising host compatibility.

## Clone and start

Take the playground URL from the user's request or current page. Use its actual
**origin** (scheme + hostname + optional port, no path) for `--allow-origin`.
For the intended Pages URL the origin is `https://openswiftuiproject.github.io`.
For a local preview use that page's actual origin, not a fixed development port.

Run the included helper from this extracted skill folder; no clone is needed first:

```sh
bash scripts/bootstrap.sh \
  --allow-origin https://openswiftuiproject.github.io
```

It clones the full URL above on `main` into `~/Developer/ai-passport`. Use
`--directory /chosen/path/ai-passport` for another location. Existing matching
`main` checkouts are reused without modifying work; an unrelated directory or
branch is refused. Never overwrite one to make the setup fit. If an existing
checkout is outdated, preserve changes before updating it or choose a new location.

Equivalent commands for an empty destination, also usable when only SKILL.md
was downloaded and the helper is absent:

```sh
git clone --branch main --single-branch \
  https://github.com/OpenSwiftUIProject/ai-passport.git ai-passport
cd ai-passport
./tools/playground/start-compiler.sh \
  --allow-origin https://openswiftuiproject.github.io
```

The launcher uses **this Passport checkout**, downloads the matching OpenSwiftUI
and LVGL sources, verifies WASI SDK 34 libraries, installs locked editor packages,
and caches shared compilation. There is no second Passport revision checkout.
Keep the service terminal running; Ctrl-C stops it. Use `--port 4201` if needed
and set the website's Compiler URL to the same port.

## Connect and verify

Open the requested playground, fill the compiler URL, then click Connect.
Allow the browser's local/loopback network permission when requested. A supplied
`?compiler=...` only prefills the field; it does not authorize automatic source
submission. Confirm that the page says Connected, edit a literal Text in
ContentView, and verify the new text appears after compilation. Test UP/DOWN/OK
on the example; Disconnect should leave the compiled preview interactive.

If connection fails, check `http://127.0.0.1:4191/health`, the terminal port and
allowed origins. HTTPS pages can encounter browser-specific loopback permissions;
use the local editor `http://127.0.0.1:4191/` as a fallback. Do not disable browser
security, bind the compiler publicly or add a tunnel. The compiler is for trusted
local development and runs as the user, not in a public compilation sandbox.

Report the checkout path, selected Swift version, compiler URL, allowed origin,
actual build/browser results and any remaining compatibility issue. This workflow
does not flash a device, publish Pages, commit or push code.

## Optional firmware and Simulator

When the user wants firmware download or simulation, read the complete ESP-IDF 5.5.3 and Simulator setup commands in `tools/playground/LOCAL_COMPILER.md` after cloning. Check Swift Embedded RISC-V libraries and reuse IDF_PATH / IDF_TOOLS_PATH, or install into the sibling toolchains directory as documented. Restart the compiler and reconnect. After successful preview, Build firmware, verify matching source, then Download full.bin or Send to Simulator. The published Playground defaults to https://openswiftuiproject.github.io/FoloToy-Passport-Simulator/, which runs in the browser without a local Simulator installation. Keep that online URL for local previews too. Open Simulator transfers the image across origins using browser window messages; keep Playground open until delivery completes. If the browser blocks a popup, allow it and click Open again, or download and select the full image manually. Only clone and start the local Node Simulator when the user needs networking or local API handoff; use the complete clone URL and commands in LOCAL_COMPILER.md. WASM is not device firmware; do not flash automatically.
