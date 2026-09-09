#!/usr/bin/env python3
"""Local feasibility build: real OpenSwiftUI Embedded sources + Passport LVGL sink."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parent
REPOSITORY = ROOT.parents[1]
WORKSPACE = REPOSITORY.parent
CACHE = REPOSITORY / 'build/playground'
PASSPORT = Path(os.environ.get('PASSPORT_SOURCE_DIR', REPOSITORY))
OSUI = Path(os.environ.get('OPENSWIFTUI_SOURCE_DIR', CACHE / 'dependencies/OpenSwiftUI'))
LVGL = Path(os.environ.get('LVGL_SOURCE_DIR', CACHE / 'dependencies/lvgl'))
SWIFTC = Path(os.environ.get('SWIFTC', Path.home() / 'Library/Developer/Toolchains/swift-6.3.1-RELEASE.xctoolchain/usr/bin/swiftc'))
BIN = SWIFTC.parent
SYSROOT = Path(os.environ.get('WASI_SYSROOT', WORKSPACE / 'toolchains/wasi-sysroot-34.0'))
BUILTINS = Path(os.environ.get('WASI_BUILTINS', WORKSPACE / 'toolchains/libclang_rt-34.0/wasm32-unknown-wasip1/libclang_rt.builtins.a'))
BUILD = Path(os.environ.get('PLAYGROUND_BUILD_DIR', CACHE / 'build'))

def run(args):
    subprocess.run(list(map(str, args)), check=True)

def swift_flags():
    return [SWIFTC, '-target', 'wasm32-unknown-none-wasm', '-module-cache-path', BUILD / 'module-cache',
            '-enable-experimental-feature', 'Embedded', '-wmo', '-Osize', '-parse-as-library', '-DOPENSWIFTUI_LVGL']

def prepare():
    BUILD.mkdir(parents=True, exist_ok=True)
    sources = [OSUI / line for line in (OSUI / 'Embedded/sources.txt').read_text().splitlines() if line]
    stamp = hashlib.sha256(b''.join(p.read_bytes() for p in sources) + str(SWIFTC).encode()).hexdigest()
    marker = BUILD / 'osui.stamp'
    if not marker.exists() or marker.read_text() != stamp:
        flags = swift_flags() + ['-package-name', 'OpenSwiftUI', '-module-name', 'OpenSwiftUI']
        macros = sorted(set(re.findall(r'@available\((OpenSwiftUI_\w+|_distantFuture)', '\n'.join(p.read_text() for p in sources))))
        for macro in macros:
            flags += ['-enable-experimental-feature', f'AvailabilityMacro={macro}:macOS 10.15, iOS 13.0']
        run(flags + ['-emit-module', '-emit-module-path', BUILD / 'OpenSwiftUI.swiftmodule', '-emit-object', '-o', BUILD / 'OpenSwiftUI.o'] + sources)
        marker.write_text(stamp)
    toolchain = BUILD / 'wasi.cmake'
    toolchain.write_text(f'''set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR wasm32)
set(CMAKE_C_COMPILER "{BIN / 'clang'}")
set(CMAKE_C_COMPILER_TARGET wasm32-unknown-wasip1)
set(CMAKE_CXX_COMPILER "{BIN / 'clang++'}")
set(CMAKE_CXX_COMPILER_TARGET wasm32-unknown-wasip1)
set(CMAKE_ASM_COMPILER "{BIN / 'clang'}")
set(CMAKE_ASM_FLAGS "--target=wasm32-unknown-wasip1")
set(CMAKE_SYSROOT "{SYSROOT}")
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_AR "{BIN / 'llvm-ar'}")
set(CMAKE_RANLIB "{BIN / 'llvm-ranlib'}")
''')
    run(['cmake', '-S', LVGL, '-B', BUILD / 'lvgl', '-G', 'Ninja',
         f'-DCMAKE_TOOLCHAIN_FILE={toolchain}', '-DCMAKE_BUILD_TYPE=MinSizeRel',
         f'-DLV_BUILD_CONF_PATH={PASSPORT / "tests/openswiftui-lvgl/lv_conf.h"}',
         '-DCONFIG_LV_BUILD_EXAMPLES=OFF', '-DCONFIG_LV_BUILD_DEMOS=OFF', '-DCONFIG_LV_USE_THORVG_INTERNAL=OFF'])
    run(['cmake', '--build', BUILD / 'lvgl', '--target', 'lvgl', '--parallel', '8'])
    includes = [PASSPORT / p for p in ['tests/openswiftui-lvgl/include', 'tests/swift-interop/include', 'main',
                 'components/bsp/include', 'tests/openswiftui-lvgl']] + [LVGL]
    flags = [BIN / 'clang', '--target=wasm32-unknown-wasip1', f'--sysroot={SYSROOT}', '-std=c11', '-Oz',
             '-ffunction-sections', '-fdata-sections', '-DLV_CONF_INCLUDE_SIMPLE', '-DLV_KCONFIG_IGNORE']
    flags += [f'-I{p}' for p in includes]
    for source in [PASSPORT / 'main/openswiftui_scene.c', PASSPORT / 'main/ui_pixel.c', PASSPORT / 'main/ui_pixel_math.c', ROOT / 'preview.c']:
        run(flags + ['-c', source, '-o', BUILD / (source.stem + '.o')])

def compile_view(source, output):
    started = time.monotonic()
    obj = output.with_suffix('.client.o')
    flags = swift_flags() + ['-module-name', 'PassportPreview', '-I', BUILD, '-Xcc', f'-I{PASSPORT / "tests/swift-interop/include"}',
        '-import-bridging-header', PASSPORT / 'main/swift/PassportBridge.h', '-emit-object', '-o', obj]
    run(flags + [source, PASSPORT / 'main/swift/PassportSceneSink.swift', ROOT / 'PreviewHost.swift'])
    run([BIN / 'wasm-ld', '--no-entry', '--gc-sections', '--strip-debug', '-z', 'stack-size=1048576',
         '--initial-memory=4194304', '--max-memory=33554432',
         '--export=preview_init', '--export=preview_button', '--export=preview_tick',
         '--export=preview_framebuffer', '--export=preview_revision',
         obj, BUILD / 'OpenSwiftUI.o', BUILD / 'openswiftui_scene.o', BUILD / 'ui_pixel.o',
         BUILD / 'ui_pixel_math.o', BUILD / 'preview.o', BUILD / 'lvgl/lib/liblvgl.a',
         '-L', SYSROOT / 'lib/wasm32-wasip1', '-lc', '-lm', BUILTINS, '-o', output])
    obj.unlink()
    return {'seconds': round(time.monotonic() - started, 3), 'bytes': output.stat().st_size}

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prepare', action='store_true')
    parser.add_argument('--source', type=Path, default=ROOT / 'ContentView.swift')
    parser.add_argument('--output', type=Path, default=BUILD / 'preview.wasm')
    args = parser.parse_args()
    try:
        if args.prepare: prepare()
        print(json.dumps(compile_view(args.source.resolve(), args.output.resolve())))
    except subprocess.CalledProcessError as error:
        # clang/swiftc already emitted the useful diagnostics; omit Python's traceback.
        sys.exit(error.returncode)
