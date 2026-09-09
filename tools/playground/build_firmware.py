#!/usr/bin/env python3
"""Build and verify a full ESP32-C3 image for a submitted ContentView."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import build


def run(args, env):
    subprocess.run(list(map(str, args)), cwd=build.REPOSITORY, env=env, check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    directory = build.REPOSITORY / 'build/playground/firmware-cache'
    directory.mkdir(parents=True, exist_ok=True)
    source = args.source.read_bytes()
    # Only the compiler service's serialized firmware worker writes this file.
    view = directory / 'ContentView.swift'
    view.write_bytes(source)
    env = dict(os.environ, SDKCONFIG_DEFAULTS=str(build.REPOSITORY / 'sdkconfig.defaults'))
    command = ['idf.py', '-B', directory, '-D', f'SDKCONFIG={directory / "sdkconfig"}',
               '-D', f'PASSPORT_CONTENT_VIEW_SOURCE={view}', '-D', 'PASSPORT_BOOT_2048=OFF',
               '-D', 'PASSPORT_2048_SOAK=OFF', '-D', f'OPENSWIFTUI_SOURCE_DIR={build.OSUI}']
    run(command + ['build'], env)
    run(command + ['merge-bin', '-o', directory / 'FoloToy-AI-Passport-full.bin'], env)
    run([sys.executable, build.REPOSITORY / 'tools/verify_firmware.py', directory], env)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(directory / 'FoloToy-AI-Passport-full.bin', args.output)
    print(json.dumps({'sourceSha256': hashlib.sha256(source).hexdigest(),
                      'sha256': hashlib.sha256(args.output.read_bytes()).hexdigest(),
                      'bytes': args.output.stat().st_size}), flush=True)


if __name__ == '__main__':
    try: main()
    except subprocess.CalledProcessError as error: sys.exit(error.returncode)
