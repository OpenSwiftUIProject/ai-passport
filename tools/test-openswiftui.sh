#!/usr/bin/env bash
set -euo pipefail
repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
osui_root="${OPENSWIFTUI_SOURCE_DIR:-${repo_root}/../framework/OpenSwiftUI}"
if [[ ! -f "${osui_root}/Embedded/sources.txt" ]]; then
    echo "ERROR: OpenSwiftUI Embedded worktree missing; see README.md." >&2
    exit 1
fi
"${osui_root}/Scripts/test_embedded.sh"
test_dir="$(mktemp -d "${TMPDIR:-/tmp}/passport-openswiftui.XXXXXX")"
trap 'rm -rf -- "${test_dir}"' EXIT
python3 "${osui_root}/Scripts/build_embedded.py" --target host --output "${test_dir}"
cd "${repo_root}"
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -Itests/swift-interop/include -Imain/swift \
    -c tests/openswiftui-interop/harness.c -o "${test_dir}/harness.o"
host_flags=()
if [[ "$(uname -s)" == Darwin ]]; then
    host_flags=(-sdk "$(xcrun --show-sdk-path)")
fi
swiftc -enable-experimental-feature Embedded -wmo -Osize -parse-as-library \
    "${host_flags[@]}" -I "${test_dir}" -Xcc -Itests/swift-interop/include \
    -import-bridging-header main/swift/PassportBridge.h \
    main/swift/PassportCore/PassportState.swift main/swift/PassportDemo.swift \
    main/swift/ContentView.swift main/swift/PassportSceneSink.swift main/swift/PassportContent.swift \
    "${test_dir}/harness.o" "${test_dir}/libOpenSwiftUI.a" -o "${test_dir}/interop"
"${test_dir}/interop"
