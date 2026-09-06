#!/usr/bin/env bash
# Uses the real LVGL/C scene on the host. Does not communicate with the board.
set -euo pipefail
if [[ $# -ne 1 ]]; then
    echo "Usage: $0 <output.png> (run through tools/with-env.sh)" >&2
    exit 2
fi
repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
osui_root="${OPENSWIFTUI_SOURCE_DIR:-${repo_root}/../framework/OpenSwiftUI}"
preview_build="${PASSPORT_PREVIEW_BUILD_DIR:-${repo_root}/../work/openswiftui-preview}"
output="$1"
mkdir -p "${preview_build}"
cmake -S "${repo_root}/managed_components/lvgl__lvgl" -B "${preview_build}/lvgl" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DLV_BUILD_CONF_PATH="${repo_root}/tests/openswiftui-lvgl/lv_conf.h" \
    -DCONFIG_LV_BUILD_EXAMPLES=OFF -DCONFIG_LV_BUILD_DEMOS=OFF -DCONFIG_LV_USE_THORVG_INTERNAL=OFF
cmake --build "${preview_build}/lvgl" --target lvgl --parallel 8
python3 "${osui_root}/Scripts/build_embedded.py" --target host --output "${preview_build}/swift"
cd "${repo_root}"
includes=(-Itests/openswiftui-lvgl/include -Itests/swift-interop/include -Imain -Imain/swift \
    -Icomponents/bsp/include -Imanaged_components/lvgl__lvgl -Itests/openswiftui-lvgl \
    -DLV_CONF_INCLUDE_SIMPLE -DLV_KCONFIG_IGNORE)
for source in main/demo_openswiftui.c main/ui_pixel.c main/ui_pixel_math.c main/screen_protocol.c tests/openswiftui-lvgl/preview.c; do
    "${CC:-cc}" -std=c11 -O2 "${includes[@]}" -c "${source}" -o "${preview_build}/$(basename "${source}" .c).o"
done
host_flags=()
if [[ "$(uname -s)" == Darwin ]]; then
    host_flags=(-sdk "$(xcrun --show-sdk-path)")
fi
swiftc -enable-experimental-feature Embedded -wmo -Osize -parse-as-library \
    "${host_flags[@]}" -I "${preview_build}/swift" -Xcc -Itests/swift-interop/include \
    -import-bridging-header main/swift/PassportBridge.h \
    main/swift/PassportCore/PassportState.swift main/swift/PassportDemo.swift \
    main/swift/ContentView.swift main/swift/PassportSceneSink.swift main/swift/PassportContent.swift \
    "${preview_build}/demo_openswiftui.o" "${preview_build}/ui_pixel.o" "${preview_build}/ui_pixel_math.o" \
    "${preview_build}/screen_protocol.o" "${preview_build}/preview.o" \
    "${preview_build}/swift/libOpenSwiftUI.a" "${preview_build}/lvgl/lib/liblvgl.a" \
    -o "${preview_build}/preview"
"${preview_build}/preview" "${preview_build}/frame.fps1"
python3 - "${preview_build}/frame.fps1" "${output}" <<'PYTHON'
from pathlib import Path
import sys
sys.path.insert(0, "tools")
from capture_screen import Frame, save_png
output = Path(sys.argv[2]).expanduser().resolve()
for wire_suffix, image_suffix in [("", ""), (".down", "-down"), (".hidden", "-hidden")]:
    frame = Frame("12345678")
    for line in Path(sys.argv[1] + wire_suffix).read_bytes().splitlines():
        frame.accept(line)
    destination = output.with_name(output.stem + image_suffix + output.suffix)
    save_png(frame, destination)
    print("Host LVGL preview saved:", destination)
PYTHON
