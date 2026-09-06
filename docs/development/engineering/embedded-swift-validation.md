[简体中文](embedded-swift-validation.zh_CN.md) · English

# Embedded Swift validation record

Recorded on **2026-09-06** for the local `feature/embedded-swift-starter` branch,
based on upstream `c73254e2f6a142bcafca2683056845c6337cafb2`. This records the
local starter implementation, not a published release.

| Check | Result |
| --- | --- |
| Environment | PASS: macOS 26.6.2 / Apple Silicon, Swift 6.3.1 RELEASE, ESP-IDF 5.5.3, GCC 14.2.0, Python 3.12.13, CMake 4.4.0 |
| Swift model host tests | PASS: 3 XCTest cases |
| Embedded Swift C interop | PASS: real Swift adapter, fake C endpoints, 200 page lifecycles across valid/unavailable battery scenarios |
| Existing C/Python tests | PASS: UI math and 2 firmware-verifier tests |
| Screenshot host tests | PASS: firmware C encoder consumed by Python; 5 cases covering pixel/PNG colors, CRC, bounds, stale records, gaps and overlaps |
| USB screenshots | PASS: 20 reconnect/capture cycles; 76,800 pixels per frame; 0.557-0.567 s including completion diagnostics |
| Screenshot memory | PASS in the 20-capture run: free heap remained 151,736 bytes; minimum task stack margin 4,520 bytes |
| Interrupted receiver / malformed input | PASS: close mid-frame, reconnect, reject malformed/overlong commands, then capture a complete frame without reboot |
| Repository/workflow/static checks | PASS |
| Skill schema | PASS |
| ESP32-C3 firmware | PASS: RISC-V 32-bit ELF, linked Swift lifecycle functions and C BSP |
| Complete gate | PASS: `tools/with-env.sh ./tools/validate.sh` |
| USB deployment / boot | PASS: ESP32-C3 revision v1.1, 8 MB Flash; app-only write and digest verification; Embedded Swift startup marker observed |
| BSP initialization | PASS: device reports Display=1, Button=1, Audio=1, Battery=1; this is not visual/audio acceptance |
| Initial firmware idle observation | PASS: 240.2 seconds of serial capture, one startup, zero panic/watchdog markers; no Swift page input events observed |
| Swift page entry | PASS: user confirms the test page opens normally; user report after the serial capture ended |
| Counter / backlight / long-OK acceptance | PENDING: these specific behaviors have not yet been confirmed |
| Permanent Recovery | UNAVAILABLE before flashing: no partition entry and the entire reserved region was erased |
| Remote GitHub CI | NOT RUN: inherited environments still need Swift provisioning |

## Screenshot firmware

The deployed screenshot app is **1,531,472 bytes**, within the **3,145,728-byte**
factory partition; its merged artifact is **1,597,008 bytes**.

```text
build/FoloToy-AI-Passport-full.bin
Merged SHA-256: fd28679a152c003a41acb03d94b82c8b84d6a5855b33b87ad413aeb2f77bf368
Application SHA-256: 0e23cc1939f7daa88b97393249512b52d05dc9e7931f34091e83a40f606d04b4
```

The complete gate passed before deployment. Only the application at `0x10000`
was updated, with device digest checks before and after confirming the original
bootloader/table/NVS and `0x310000..0x7fffff` data remain unchanged. The final
host receiver also passed the complete static gate.

To prevent `USB_UART_CHIP_RESET` when connecting on macOS, the receiver follows
ESP-IDF Monitor's no-reset order:
assert both before opening, then release RTS before DTR. Three connection-only
cycles and 20 capture/reconnect cycles produced no boot or panic markers.

The mid-frame receiver-close check produced a bounded `ESP_ERR_TIMEOUT`; a
fresh request recovered all 76,800 pixels in 0.684 seconds with unchanged heap.
This verifies stopped-receiver recovery, not physical USB cable removal.
No 150 KiB screenshot buffer is allocated on the MCU. The test captured the
actual LVGL main menu and inspected the PNG's colors, orientation and layout;
physical brightness/panel defects remain outside the screenshot's scope.

Private evidence is in
`FoloToy/work/device-backups/20260906T044501Z/screenshot-fd28679a/`, including
`boot.log`, `stress-results.json`, `serial-open-check.json`, and
`interruption-results.json`. Build and host logs are in
`FoloToy/work/logs/usb-screenshot-full-validation.log` and
`FoloToy/work/logs/usb-screenshot-static.log`.

## Initial Swift firmware

The initial app was **1,519,392 bytes**; its merged file was **1,584,928 bytes**.
It is retained for rollback:

```text
../work/deployment/swift-aa3fe0d3/FoloToy-AI-Passport-full.bin
SHA-256: aa3fe0d3ace2697fd15f0dedae8d15c39b6b2a75b748032ab4d8b59825a1fc2f
```

The full gate verified partition MD5/layout, the application offset `0x10000`,
the 3 MB limit, protected identity/Recovery ranges and payload exclusion, and
the linked five-second Recovery boot hook in the community artifact.

## USB deployment on 2026-09-06

A complete 8,388,608-byte device backup was read and hashed before writing.
Secure Boot and Flash Encryption were disabled. The original factory layout
contains `imgstore`, `imgframe`, `cardid`, `audio`, and `imgava`, but no Recovery
entry; the entire region `0x700000..0x7fffff` was `0xff` before deployment.

Only the verified application was flashed at `0x10000`; the original bootloader
and partition table remain installed. The application SHA-256 is
`d07b6538cb67e86fc086954a1b02ee87b2712ed2ccb80ad8f1e0e5a6cc9a3bfe`.
Device digest verification confirmed that `0x0..0xffff` and
`0x310000..0x7fffff` still match the backup. These ranges include the original
bootloader/table/NVS, all resources, identity, and the empty Recovery region.

The original bootloader loaded the new app from `0x10000`. Logs identify
ESP-IDF 5.5.3 and contain `Embedded Swift ready on FoloToy ESP32-C3`, followed
by successful initialization of Display, Button, Audio, and Battery. The
community bootloader's Recovery hook was not installed or tested on this unit.

Private evidence is outside Git in
`FoloToy/work/device-backups/20260906T044501Z/`: the full backup, manifest,
`flash.log`, `preservation.log`, and `swift-runtime.log`. Deployment segments
were extracted from the recorded merged artifact under
`FoloToy/work/deployment/swift-aa3fe0d3/`; this avoids mixing the isolated
validation build with later incremental build segments.

The initial incremental `idf.py size` report showed 179,112 bytes of static DRAM usage.
Its reported 142,184-byte remainder is a link-time budget, **not measured runtime
free heap**. Long-duration fragmentation and combined peripheral workloads
remain unverified; the screenshot-run heap measurements are recorded above.

The map contains `passport_swift_prepare`, `passport_swift_enter`,
`passport_swift_action`, `passport_swift_exit`, `bsp_display_backlight`, and
`bsp_battery_soc`; Swift is part of the firmware image, not just the host package.

Local evidence is saved outside Git under `FoloToy/work/logs/`:
`idf-install.log`, `swift-firmware-build.log`, and `swift-full-validation.log`.
Generated binaries, ELF, and map are in the ignored `build/` directory.

The known version-macro and Recovery size diagnostics are explained in the
[starter README](../../../README.md). Follow its hardware checklist before
claiming screen/input/backlight/battery behavior or Recovery works on a device.
