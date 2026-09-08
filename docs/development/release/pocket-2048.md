<p align="right">
  <a href="pocket-2048.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Pocket 2048 release

An animated number-merging puzzle: slide equal tiles together, build your score,
and try to reach 2048. The release starts directly in the game.

## How to play

| Button | Vertical mode (initial) | Horizontal mode |
| --- | --- | --- |
| UP | Move up | Move left |
| DOWN | Move down | Move right |
| Short OK | Switch to horizontal | Switch to vertical |
| Long OK | Leave the game for the demo menu | Leave the game for the demo menu |

The footer shows the current mapping. Switching axes does not move or add tiles.
A successful move adds a new tile; matching tiles merge once per move. Sliding
and merge animations help you follow each turn. After reaching 2048 or running
out of moves, press OK to start again. Leaving the game or powering off loses
the current round. There is no save, undo, leaderboard or network requirement.

## Choose the correct file

- `*-full.bin`: the merged community artifact. Select this for the official
  AI Passport community upload. The supported official installer interprets it
  and preserves device identity and factory Recovery; the image does not include
  either device-specific identity data or a replacement Recovery.
- `*-app.bin`: application only, for developers with a verified compatible
  partition table and application at `0x10000` (maximum `0x300000` bytes).
  Do not upload this file to the community.
- `manifest.json`: exact source revisions, build configuration and binary hashes.
- `SHA256SUMS`: integrity checks for the extracted files. On macOS run
  `shasum -a 256 -c SHA256SUMS`; on Linux run `sha256sum -c SHA256SUMS`.

Use the official community/mini-program installation instructions for a device
with factory Recovery. This candidate's community installation has not yet been
tested. A legacy TRAE gift device may have a different partition layout and no
Recovery: do not erase it or write the full image from `0x0`. For such a device,
first back up its full flash privately and verify the layout; only then write
`*-app.bin` at `0x10000`, checking that all bytes outside the application remain
unchanged. No device backup is included in this distribution.

## Reproduce the candidate

Clone `https://github.com/OpenSwiftUIProject/ai-passport` and check out the
`source.revision` in `manifest.json`. Follow its root README to install the
ESP-IDF and Embedded Swift toolchains and set up OpenSwiftUI. Check out the
manifest's `openswiftui.revision` in that repository, then run:

```bash
tools/with-env.sh tools/validate.sh --release-2048
```

This runs the complete host gate and builds from isolated defaults with
`PASSPORT_BOOT_2048=ON` and `PASSPORT_2048_SOAK=OFF`. It checks the merged image,
protected regions, application size and Recovery hook, then creates a ZIP and
checksums under `../work/releases/pocket-2048-<revision>/`. The source trees must
be clean and the destination must be new. Build timestamps and local toolchain
paths can affect binary bytes; the manifest identifies the supplied artifact.
The ordinary `tools/validate.sh` gate retains the static demo startup.

## Publisher capture

The firmware supports the official `FAP_SCREENSHOT_V1` USB capture request as
well as the existing FPS1 tool. It streams one full RGB565 frame while holding
LVGL and the console lock, using the partial drawing buffer without allocating
a full framebuffer. A transfer has an eight-second deadline; closing the host
must not leave the UI blocked indefinitely. Capture redraws on a worker with
the configured LVGL stack budget plus 2 KiB. It does not change the game state,
reset the board, alter settings or read device credentials.

Install the official publisher skill from
`https://ai-passport.folotoy.cn/skills/folotoy-ai-passport-publisher.zip` and follow
its `capture-screen`, `validate`, authorization and preview workflow. A fresh
real-device capture and its receipt are required for the cover. Host previews
are test fixtures, not serial connection evidence. Upload only after reviewing
and approving the complete submission preview.
