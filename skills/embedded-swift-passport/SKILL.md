---
name: embedded-swift-passport
description: Develop and test Embedded Swift firmware for this FoloToy AI Passport ESP32-C3 project using its C BSP and LVGL bridge.
---
[简体中文](SKILL.zh_CN.md) · English

# Embedded Swift on AI Passport

Use this skill for firmware changes in this project. Read the root `AGENTS.md`
and [starter README](../../README.md) first. The official content-push skill
controls the factory application's content queue; it is a separate integration.

- Use ESP-IDF **5.5.3**, `espressif/idf_swift` **1.0.1**, and an Embedded-capable
  Swift toolchain with `riscv32-none-none-eabi` libraries. The locally selected
  toolchain and setup commands are documented in the README.
- Keep pure logic in `main/swift/PassportCore`, firmware entry points in
  `main/swift/PassportDemo.swift`, C UI adapters in `main/demo_swift.c`, and actual
  board drivers in `components/bsp`. SwiftPM tests and ESP-IDF compile the same
  core source; do not create a second model for the host.
- `PassportBridge.h` is the C/Swift contract. Retain its fixed-width parameters
  and C linkage. Prefer small C wrappers for variadic macros or APIs that do not
  import cleanly. Direct BSP calls work for ordinary C functions.
- `prepare` runs outside LVGL during startup. Page entry/action/exit are called
  under the menu's LVGL lock. Do not introduce blocking I2C, audio, storage, or
  network operations into input callbacks. The current battery value is a boot
  snapshot, deliberately sampled before UI startup.
- Retain the official menu and its OK-long return behavior unless the user
  requests different navigation. Restore baseline backlight on page exit.
- Keep the protected identity/Recovery regions, boot hook, and 3 MB application
  limit defined in the existing firmware compatibility document.

## Verify the boundary that changed

1. `swift test` exercises hardware-independent logic on the host.
2. `tools/with-env.sh tools/test-swift-interop.sh` compiles the real adapter in
   Embedded mode and executes it with fake C BSP/UI endpoints on the host.
3. `tools/with-env.sh ./tools/validate.sh` runs the complete repository, host,
   ESP32-C3 build, and merged-image compatibility checks.
4. Follow the README's physical checklist once a device is available. Report
   host execution, cross-compilation, and board observations separately.

Do not claim a host fake or a RISC-V build exercised the physical display, ADC,
or battery. Runtime logs, screen behavior, and recovery entry need a device.
Keep personal device credentials out of source and documentation. The setup
commands do not authorize a push, publication, or destructive flash operation.
