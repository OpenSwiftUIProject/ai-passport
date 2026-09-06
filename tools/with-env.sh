#!/usr/bin/env bash
# Activate the workspace-local IDF and an Embedded-capable Swift toolchain only
# for this command. Does not modify shell startup files or global settings.
set -eo pipefail
repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
workspace_root="$(dirname -- "${repo_root}")"
export IDF_PATH="${IDF_PATH:-${workspace_root}/toolchains/esp-idf-v5.5.3}"
export IDF_TOOLS_PATH="${IDF_TOOLS_PATH:-${workspace_root}/toolchains/espressif}"

if [[ -n "${SWIFT_TOOLCHAIN:-}" ]]; then
    swift_bin="${SWIFT_TOOLCHAIN}/usr/bin"
elif [[ -d "${HOME}/Library/Developer/Toolchains/swift-6.3.1-RELEASE.xctoolchain" ]]; then
    swift_bin="${HOME}/Library/Developer/Toolchains/swift-6.3.1-RELEASE.xctoolchain/usr/bin"
else
    swift_bin="$(dirname -- "$(command -v swiftc)")"
fi
if [[ ! -d "${swift_bin}/../lib/swift/embedded/riscv32-none-none-eabi" ]]; then
    echo "ERROR: Select a RISC-V Embedded Swift toolchain via SWIFT_TOOLCHAIN." >&2
    exit 1
fi
if [[ ! -f "${IDF_PATH}/export.sh" ]]; then
    echo "ERROR: Install ESP-IDF 5.5.3 first; see README.md." >&2
    exit 1
fi
# ESP-IDF activation needs Python; reuse Homebrew's installed 3.12 on macOS.
if [[ -x /opt/homebrew/opt/python@3.12/bin/python3.12 ]]; then
    export PATH="/opt/homebrew/opt/python@3.12/libexec/bin:${PATH}"
fi
source "${IDF_PATH}/export.sh" >/dev/null
if [[ "$(idf.py --version)" != "ESP-IDF v5.5.3" ]]; then
    echo "ERROR: This firmware requires ESP-IDF v5.5.3." >&2
    exit 1
fi
export PATH="${swift_bin}:${PATH}"
cd "${repo_root}"
if [[ $# -eq 0 ]]; then
    echo 'Usage: tools/with-env.sh <command> [arguments...]' >&2
    exit 2
fi
exec "$@"
