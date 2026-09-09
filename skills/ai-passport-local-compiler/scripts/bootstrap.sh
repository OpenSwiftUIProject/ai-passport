#!/usr/bin/env bash
# This script can run from an extracted skill ZIP; it needs no existing checkout.
set -euo pipefail
repo_url='https://github.com/OpenSwiftUIProject/ai-passport.git'
checkout_dir="${HOME}/Developer/ai-passport"
service_args=()
while [[ $# -gt 0 ]]; do
    case "$1" in
        --directory)
            [[ $# -ge 2 ]] || { echo 'Missing --directory value' >&2; exit 2; }
            checkout_dir="$2"; shift 2 ;;
        --help|-h)
            echo 'Usage: bash scripts/bootstrap.sh [--directory PATH] [--allow-origin ORIGIN] [--port PORT] [--prepare-only]'
            echo "Clones ${repo_url} on main into ~/Developer/ai-passport by default."
            exit 0 ;;
        *) service_args+=("$1"); shift ;;
    esac
done
command -v git >/dev/null || { echo 'Install Git first; see SKILL.md.' >&2; exit 1; }
if [[ ! -e "${checkout_dir}" ]]; then
    mkdir -p "$(dirname -- "${checkout_dir}")"
    git clone --branch main --single-branch "${repo_url}" "${checkout_dir}"
else
    actual_root="$(git -C "${checkout_dir}" rev-parse --show-toplevel 2>/dev/null || true)"
    requested_root="$(cd -- "${checkout_dir}" && pwd -P)"
    [[ "${actual_root}" == "${requested_root}" ]] || { echo 'Choose an empty --directory; this path is not a repository root.' >&2; exit 1; }
    [[ "$(git -C "${checkout_dir}" branch --show-current)" == main ]] || { echo 'Choose another --directory; the existing checkout is not on main.' >&2; exit 1; }
    git -C "${checkout_dir}" config --get-regexp '^remote\..*\.url$' |
        grep -Eq ' (https://github\.com/OpenSwiftUIProject/ai-passport(\.git)?|git@github\.com:OpenSwiftUIProject/ai-passport\.git)$' || {
            echo 'Choose another --directory; this is not the OpenSwiftUIProject/ai-passport fork.' >&2; exit 1;
        }
    echo "Reusing ${checkout_dir} without pulling, checking out or changing existing work."
fi
launcher="${checkout_dir}/tools/playground/start-compiler.sh"
if [[ ! -f "${launcher}" ]]; then
    echo 'This checkout does not contain tools/playground/start-compiler.sh. Update this fork checkout to the latest main, preserving existing work.' >&2
    exit 1
fi
exec bash "${launcher}" "${service_args[@]}"
