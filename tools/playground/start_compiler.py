#!/usr/bin/env python3
"""Prepare pinned preview dependencies and start your local Swift compiler."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parent
REPOSITORY = ROOT.parents[1]
DEPENDENCIES = {
    'OPENSWIFTUI_SOURCE_DIR': ('OpenSwiftUI', 'https://github.com/OpenSwiftUIProject/OpenSwiftUI.git',
                              'e35b91a31789c3c277e9a784b9f9b86e5eb708f5'),
    'LVGL_SOURCE_DIR': ('lvgl', 'https://github.com/lvgl/lvgl.git',
                       '85aa60d18b3d5e5588d7b247abf90198f07c8a63'),  # v9.5.0
}


def run(args, **kwargs):
    return subprocess.run(list(map(str, args)), check=True, **kwargs)


def dependency(cache, name, url, revision):
    target = cache / name
    if not target.exists():
        print(f'Fetching {name} at {revision[:12]}…', flush=True)
        with tempfile.TemporaryDirectory(prefix=f'{name}-', dir=cache) as temporary:
            clone = Path(temporary) / 'source'
            run(['git', 'init', '-q', clone])
            run(['git', '-C', clone, 'remote', 'add', 'origin', url])
            run(['git', '-C', clone, 'fetch', '--depth=1', 'origin', revision])
            run(['git', '-C', clone, 'checkout', '-q', '--detach', 'FETCH_HEAD'])
            clone.rename(target)
    head = run(['git', '-C', target, 'rev-parse', 'HEAD'], capture_output=True, text=True).stdout.strip()
    dirty = run(['git', '-C', target, 'status', '--porcelain'], capture_output=True, text=True).stdout.strip()
    if head != revision or dirty:
        raise RuntimeError(f'{target} differs from the pinned dependency; use a fresh --cache-dir or an explicit source environment override')
    return target


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', type=int, default=4191)
    parser.add_argument('--allow-origin', action='append', default=[], help='Exact website origin, e.g. https://you.github.io (repeatable)')
    parser.add_argument('--cache-dir', type=Path, default=REPOSITORY / 'build/playground')
    parser.add_argument('--export-pages', type=Path, help='Prepare and export a static Pages artifact, then exit')
    parser.add_argument('--prepare-only', action='store_true', help='Prepare the compiler without starting the HTTP service')
    args = parser.parse_args()
    # Validate origin and port before downloading or building anything.
    from server import website_origin
    try:
        origins = [website_origin(origin) for origin in args.allow_origin]
    except argparse.ArgumentTypeError as error:
        parser.error(str(error))
    if not 1 <= args.port <= 65535: parser.error('port must be between 1 and 65535')
    for command in ('git', 'cmake', 'ninja', 'node', 'npm'):
        if not shutil.which(command): parser.error(f'Missing {command}; see tools/playground/LOCAL_COMPILER.md')
    node_version = run(['node', '--version'], capture_output=True, text=True).stdout.strip()
    if int(node_version.lstrip('v').split('.')[0]) < 20: parser.error('Node.js 20+ is required')
    preferred = Path.home() / 'Library/Developer/Toolchains/swift-6.3.1-RELEASE.xctoolchain/usr/bin/swiftc'
    compiler = os.environ.get('SWIFTC') or (str(preferred) if preferred.is_file() else shutil.which('swiftc'))
    if not compiler: parser.error('Install Swift 6.3.1 from https://www.swift.org/install/ and set SWIFTC')
    compiler = Path(compiler).expanduser().absolute()
    version = run([compiler, '--version'], capture_output=True, text=True).stdout.strip()
    if not re.search(r'Swift version 6\.3\.1\b', version):
        parser.error(f'Swift 6.3.1 is required; set SWIFTC to that toolchain. Found: {version}')
    if not (compiler.parent / 'wasm-ld').is_file():
        # Swiftly's PATH entry can be a proxy; the linker lives in the toolchain.
        info = json.loads(run([compiler, '-print-target-info'], capture_output=True, text=True).stdout)
        compiler = Path(info['paths']['runtimeResourcePath']).parent.parent / 'bin/swiftc'
    for name in ('clang', 'clang++', 'wasm-ld', 'llvm-ar', 'llvm-ranlib'):
        if not (compiler.parent / name).is_file(): parser.error(f'Missing {name} beside {compiler}')
    cache = args.cache_dir.expanduser().resolve()
    cache.mkdir(parents=True, exist_ok=True)
    sources = cache / 'dependencies'
    sources.mkdir(exist_ok=True)
    env = dict(os.environ, SWIFTC=str(compiler), PLAYGROUND_BUILD_DIR=str(cache / 'build'),
               PASSPORT_SOURCE_DIR=str(REPOSITORY))
    for key, (name, url, revision) in DEPENDENCIES.items():
        if key in os.environ:
            path = Path(os.environ[key]).expanduser().resolve()
            if not path.is_dir(): parser.error(f'{key} does not exist: {path}')
            print(f'Using {key} override: {path}', flush=True)
        else:
            path = dependency(sources, name, url, revision)
        env[key] = str(path)
    # The official WASI archives live beside the repository in toolchains/.
    # No ESP-IDF, device connection or firmware environment is needed.
    run([sys.executable, ROOT / 'setup.py'], env=env)
    lock_hash = hashlib.sha256((ROOT / 'package-lock.json').read_bytes()).hexdigest()
    editor_stamp = ROOT / 'node_modules/.playground-lock'
    if not editor_stamp.exists() or editor_stamp.read_text() != lock_hash:
        run(['npm', 'ci', '--no-audit', '--no-fund'], cwd=ROOT, env=env)
        editor_stamp.write_text(lock_hash)
    run(['npm', 'run', 'build:editor'], cwd=ROOT, env=env)
    run([sys.executable, ROOT / 'package_skill.py'], env=env)
    run([sys.executable, ROOT / 'build.py', '--prepare'], env=env)
    print('Compiler prepared. Dependencies and toolchain:', flush=True)
    print(json.dumps({key: env[key] for key in ['SWIFTC', 'PLAYGROUND_BUILD_DIR', 'PASSPORT_SOURCE_DIR', *DEPENDENCIES]}, indent=2), flush=True)
    if args.export_pages:
        run([sys.executable, ROOT / 'export_static.py', '--output', args.export_pages.resolve()], env=env)
        return
    if args.prepare_only: return
    command = [sys.executable, str(ROOT / 'server.py'), '--port', str(args.port)]
    for origin in origins: command += ['--allow-origin', origin]
    os.execve(sys.executable, command, env)


if __name__ == '__main__':
    try:
        main()
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        sys.exit(f'Setup failed: {error}')
