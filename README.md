[简体中文](README.zh_CN.md) · English

# AI Passport Embedded Swift Starter

An ESP32-C3 firmware experiment based on [FoloToy/ai-passport](https://github.com/FoloToy/ai-passport).
The existing hardware menu gains a **Swift** page. UP/DOWN change a counter
between 0 and 999; OK toggles the backlight between 100% and 25%; holding OK
returns to the menu and restores 100% backlight. Reopening the page resets it.
The battery percentage is explicitly a **boot snapshot**, not a live reading.

[Validation record](docs/development/engineering/embedded-swift-validation.md):
host tests, the ESP32-C3 merged-image gate, and USB deployment/boot passed.
The user confirms Swift page entry works. Counter, backlight and long-OK
acceptance is still pending.

Application state and behavior are written in Embedded Swift. Swift directly
calls `bsp_display_backlight()` and `bsp_battery_soc()`; a small C adapter builds
the LVGL page. Existing display, input, audio, and radio demos remain available.
Permanent Recovery requires prior factory provisioning; this image does not
install it. Installing this firmware replaces the TRAE factory application;
factory profile/Token synchronization is not implemented here.

## Quick start on this workspace

Run these commands from the repository:

```bash
swift test                                      # native Swift logic tests
tools/with-env.sh tools/test-swift-interop.sh     # Embedded Swift + fake C BSP
tools/with-env.sh ./tools/validate.sh             # full gate + real C3 firmware
```

For iterative firmware builds and inspecting the ELF/map:

```bash
tools/with-env.sh idf.py build
tools/with-env.sh idf.py size
```

The full gate builds with an isolated configuration and publishes only the
verified `build/FoloToy-AI-Passport-full.bin`. An incremental build also leaves
`build/FoloToy-AI-Passport.elf`, `.map`, and the app-only `.bin` for debugging.
Only the merged `-full.bin` is suitable for the mini-program installer.

### Current device deployment

The connected TRAE unit was backed up in full before flashing on 2026-09-06.
Its original partition table has no Recovery entry, and `0x700000..0x7fffff`
was entirely erased. This unit therefore cannot use the documented permanent
BLE Recovery flow in its current state.

The USB test writes **only the application at `0x10000`**, extracted from the
verified merged artifact. The original bootloader and partition table remain
in use. Post-write device digest checks matched the backup for `0x0..0xffff`
and `0x310000..0x7fffff`, covering NVS, identity, and all resource data. The
original bootloader successfully started the Embedded Swift application.

Private backups and serial logs are outside Git in `../work/device-backups/`;
staged images are in `../work/deployment/swift-aa3fe0d3/`. This app-only USB test
does not establish mini-program installation or Recovery support for this unit.

## Layout

| Path | Purpose |
| --- | --- |
| `main/swift/PassportCore/PassportState.swift` | Same pure Swift model for host and firmware |
| `main/swift/PassportDemo.swift` | C-callable Swift lifecycle and direct BSP calls |
| `main/swift/PassportBridge.h` | Fixed-width C/Swift API contract |
| `main/demo_swift.c` | LVGL objects and physical button translation |
| `Package.swift` | SwiftPM library and host tests; open in Xcode for core work |
| `tests/swift-interop/` | Actual Embedded Swift adapter exercised against fake C endpoints |
| `tools/with-env.sh` | Per-command IDF/Swift activation |
| [Embedded Swift skill](skills/embedded-swift-passport/SKILL.md) | Project workflow, also discoverable through `.agents/skills/` |

## Toolchains

- ESP-IDF **5.5.3**, ESP32-C3, 8 MB Flash, no PSRAM.
- Managed component `espressif/idf_swift` **1.0.1**; resolved versions are in `dependencies.lock`.
- Embedded Swift **6.3.1 RELEASE** is selected from the existing macOS toolchain
  installation. The Xcode-provided Swift can run native tests, but its installation
  on this Mac does not contain the required RISC-V Embedded libraries.
- CMake **3.29+**, Ninja, and Python **3.12**.

Workspace-local downloads live outside the Git repository:

```text
FoloToy/
  ai-passport/                 this checkout
  toolchains/esp-idf-v5.5.3/    ESP-IDF source
  toolchains/espressif/        compiler, Python environment and tools
  work/logs/                  setup and validation logs
```

On a fresh machine, install an Embedded-capable toolchain from
[Swift.org](https://www.swift.org/install/), CMake, Ninja, and Python first. Then:

```bash
mkdir -p ../toolchains
git clone --branch v5.5.3 --depth 1 --recurse-submodules --shallow-submodules \
  https://github.com/espressif/esp-idf.git ../toolchains/esp-idf-v5.5.3
export IDF_TOOLS_PATH="$(cd .. && pwd)/toolchains/espressif"
../toolchains/esp-idf-v5.5.3/install.sh esp32c3
```

`tools/with-env.sh` defaults to the sibling `toolchains/` directory. Override
`IDF_PATH`, `IDF_TOOLS_PATH`, or `SWIFT_TOOLCHAIN` to reuse another installation.
`SWIFT_TOOLCHAIN` is the toolchain root containing `usr/bin/swiftc`. The script
does not edit shell profiles, change Xcode selection, or install global skills.

## Test coverage and physical acceptance

Native tests cover mixed input, brightness state, and saturating counter bounds.
The interop harness executes the **real** Swift adapter in Embedded mode on the
host: 200 enter/action/exit cycles, unknown action rejection, optional battery
data, and backlight restoration. Its C endpoints are fakes. Cross-compilation
and merged-image verification exercise the actual ESP-IDF/BSP integration.
Neither of those checks proves the board boots or its peripherals work.

Once a device is connected with a data-capable USB cable:

1. Identify its actual `/dev/cu.usbmodem*` port; close other serial clients.
2. Preserve the official recovery link privately. Confirm the replacement
   firmware is intended before installing it through the official mini-program
   or [web tool](https://ai-passport.folotoy.cn/tools/web-flasher/).
3. Check for `Embedded Swift ready on FoloToy ESP32-C3` in USB serial logs.
4. From the initial Display menu selection, click UP once to select Swift, then
   OK. Confirm the counter starts at 0 and the battery shows a plausible boot
   reading or `--%`.
5. UP, UP, DOWN must show 1. OK must alternate 25%/100% backlight. Long OK must
   restore full brightness and return to the menu. Reenter and repeat 20 times.
6. Confirm no reboot loop, assertion, watchdog, growing heap loss, or damaged
   baseline demos. On units with a provisioned permanent Recovery and its boot
   hook, power off and hold UP while powering on for five seconds to verify
   Recovery entry. The current legacy unit does not meet that prerequisite.

Keep `cardid` at `0x356000`, Recovery at `0x700000`, and the 3 MB app limit.
Never use full-chip erase on the provisioned gift. See the authoritative
[BLE/recovery contract](docs/development/engineering/ble-recovery-compatibility.md).
No device SN, KEY, or personal recovery URL belongs in this repository.

## Scope

This is a local firmware starter, not a SwiftUI app or an implementation of the
factory cloud protocol. No radio/audio behavior was added to the Swift page.
The inherited GitHub workflows need Embedded Swift provisioned in their build
environment before they can build this branch; local validation is the current
supported path. Remote CI is untested; physical results are limited to the
checks explicitly recorded above.

Known build diagnostics: the pinned Swift component forwards three unused
`BUTTON_VER_*` C definitions that produce Swift warnings. No Swift logic reads
them. ESP-IDF also compares the application against the smaller protected
Recovery partition and prints a size warning. This application is placed in the
3 MB factory partition, not Recovery; the merged-image gate verifies that layout.

References: [FoloToy hardware](docs/hardware-design/AI_HARDWARE_DEVELOPMENT_GUIDE.md),
[upstream overview](docs/README.md),
[Espressif Swift component](https://components.espressif.com/components/espressif/idf_swift/versions/1.0.1/readme?language=en).
