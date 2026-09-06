#!/usr/bin/env bash
# Host execution of Embedded Swift with fake BSP/UI endpoints; not a board test.
set -euo pipefail
repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
test_dir="$(mktemp -d "${TMPDIR:-/tmp}/passport-swift-interop.XXXXXX")"
trap 'rm -rf -- "${test_dir}"' EXIT
cd "${repo_root}"
host_flags=()
if [[ "$(uname -s)" == Darwin ]]; then
    host_flags=(-sdk "$(xcrun --show-sdk-path)")
fi
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror \
    -Itests/swift-interop/include -Imain/swift \
    -c tests/swift-interop/harness.c -o "${test_dir}/harness.o"
swiftc -enable-experimental-feature Embedded -wmo -Osize -parse-as-library \
    "${host_flags[@]}" \
    -Xcc -Itests/swift-interop/include \
    -import-bridging-header main/swift/PassportBridge.h \
    main/swift/PassportCore/PassportState.swift main/swift/PassportDemo.swift \
    "${test_dir}/harness.o" -o "${test_dir}/interop"
"${test_dir}/interop"
