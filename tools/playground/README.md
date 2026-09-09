[简体中文](README.zh_CN.md) · English

# OpenSwiftUI AI Passport Playground

Write `struct ContentView: View` in the browser and preview the real OpenSwiftUI
Embedded layout, State and Passport LVGL rendering. This tool belongs to
https://github.com/OpenSwiftUIProject/ai-passport and uses that checkout's C/Swift
bridge directly. Its WASM preview does not require the QEMU Simulator repository.

The intended hosted entry is https://openswiftuiproject.github.io/ai-passport/.
Visitors can play the bundled example immediately or connect their own compiler.

## Start from scratch

Install Swift 6.3.1 RELEASE, Python 3.9+, Git, CMake, Ninja and Node.js 20+ with
npm. macOS arm64 is validated; Linux and WSL are not yet validated. Then:

```sh
git clone --branch main --single-branch \
  https://github.com/OpenSwiftUIProject/ai-passport.git ai-passport
cd ai-passport
./tools/playground/start-compiler.sh \
  --allow-origin https://openswiftuiproject.github.io
```

The launcher uses the current Passport `main` checkout, prepares OpenSwiftUI and
LVGL dependencies, and starts the compiler at `http://127.0.0.1:4191/compile`.
There is no second pinned Passport checkout. The standalone
[setup skill](https://github.com/OpenSwiftUIProject/ai-passport/tree/main/skills/ai-passport-local-compiler)
also works before cloning; the website includes its downloadable ZIP.
See [local setup](LOCAL_COMPILER.md) for prerequisites, overrides and connection help.

## Preview behavior

Click Connect before editing. Auto preview compiles 650 ms after the last edit;
Build & Run compiles immediately. Each successful build creates a new WASM worker
and resets State. UP/DOWN/OK update State within the running view. Compile errors
retain the previous preview; Disconnect preserves its interactivity. Edits stay
in the tab and are discarded when the page reloads.

The default example supports Color, literal Text, `Image("spark")`, measured
stacks, frames/padding/background and root State/button closures. It uses 240 × 320
RootGeometry with top 66, leading/trailing 12 and bottom 30 insets. The C bridge
supplies the same bitmap, fonts and LVGL scene as the firmware, with a fixed 73%
battery fixture. Full desktop SwiftUI, Foundation, SF Symbols, arbitrary uploaded
assets and firmware flashing from the browser are not implemented.

## Build and validate

Run from the repository root:

```sh
./tools/playground/start-compiler.sh --export-pages build/pages/ai-passport
python3 -m unittest discover -s tools/playground -p 'test_*.py'
node --test tools/playground/compiler-config.test.mjs
cd tools/playground
node smoke.mjs
node verify-static.mjs ../../build/pages/ai-passport
```

Choose an empty export directory. The launcher caches framework/C objects under
`build/playground/`; WASI C libraries live in the sibling `toolchains/` directory.
It downloads [OpenSwiftUI](https://github.com/OpenSwiftUIProject/OpenSwiftUI) at
`e35b91a31789c3c277e9a784b9f9b86e5eb708f5` and [LVGL](https://github.com/lvgl/lvgl)
9.5.0 at `85aa60d18b3d5e5588d7b247abf90198f07c8a63`. No OAG or ESP-IDF is needed.
Build limits and the API boundary are documented in [local setup](LOCAL_COMPILER.md).

The WASM smoke test exercises 1,000 State/input cycles and exports three frames;
the static verifier checks the artifact hashes and those same frames. To compare
with independently generated native LVGL FPS1 fixtures, run `python3 compare-host.py
/path/to/frame.fps1` after the smoke test. Existing initial/DOWN/hidden-image
fixtures matched all 76,800 pixels per frame. Host speed and memory are not device
performance measurements; no physical device is exercised by these browser tests.

For the GitHub Pages workflow and publication layout, see [deployment](DEPLOYMENT.md).

## Firmware and Simulator

After a successful preview, click Build firmware, then download the matching full.bin or Send to Simulator and Open Simulator. Firmware requires ESP-IDF 5.5.3; the Simulator needs the import API and must allow the current page origin. See [local setup](LOCAL_COMPILER.md) for the full steps and limits. This workflow does not flash hardware.
