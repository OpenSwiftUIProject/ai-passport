#!/usr/bin/env python3
"""Export a self-contained Pages-compatible example, optionally with a compile API."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
from urllib.parse import urlsplit
import build
from package_skill import package_skill

ROOT = build.ROOT

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def notices():
    sources = {
        'Passport': build.PASSPORT / 'LICENSE',
        'OpenSwiftUI': build.OSUI / 'LICENSE',
        'Swift runtime': build.SWIFTC.parent.parent / 'share/swift/LICENSE.txt',
        'LVGL': build.LVGL / 'LICENCE.txt',
        'Montserrat font': build.LVGL / 'scripts/built_in_font/font_license/Montserrat/OFL.txt',
    }
    for path in sorted((ROOT / 'licenses').glob('*.txt')):
        sources[path.name] = path
    for path in sorted((ROOT / 'node_modules').rglob('LICENSE*')):
        if path.is_file(): sources[str(path.relative_to(ROOT / 'node_modules'))] = path
    return '\n\n'.join(f'{name}\n{"=" * 72}\n{path.read_text()}' for name, path in sources.items())

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=build.REPOSITORY / 'build/pages/ai-passport')
    parser.add_argument('--compile-endpoint', help='HTTPS or loopback API URL to prefill; visitors can connect their own compiler')
    args = parser.parse_args()
    if args.compile_endpoint:
        url = urlsplit(args.compile_endpoint)
        if (not url.hostname or url.username or url.password or url.fragment or url.query
                or not (url.scheme == 'https' or (url.scheme == 'http' and url.hostname in ('localhost', '127.0.0.1')))):
            parser.error('--compile-endpoint must use HTTPS or HTTP loopback, without credentials, query or fragments')
    output = args.output.resolve()
    if output.exists() and any(output.iterdir()):
        parser.error('Output must be empty; choose a fresh export directory')
    if not (ROOT / 'node_modules').exists(): parser.error('Run npm ci in playground first')
    subprocess.run(['node', str(ROOT / 'build-editor.mjs')], check=True)
    build.compile_view(ROOT / 'ContentView.swift', build.BUILD / 'static-example.wasm')
    output.mkdir(parents=True, exist_ok=True)
    for name in ['index.html', 'styles.css', 'app.js', 'compiler-config.js', 'firmware.js', 'simulator-config.js', 'browser-handoff.js', 'worker.js', 'wasi.js',
                 'ContentView.swift', 'LOCAL_COMPILER.md', 'LOCAL_COMPILER.zh_CN.md',
                 'DEPLOYMENT.md', 'DEPLOYMENT.zh_CN.md', 'README.md', 'README.zh_CN.md']:
        shutil.copy2(ROOT / name, output / name)
    shutil.copy2(ROOT / 'build/editor.js', output / 'editor.js')
    shutil.copy2(build.REPOSITORY / 'assets/images/openswiftui-playground-social.png', output / 'social-card.png')
    shutil.copy2(build.BUILD / 'static-example.wasm', output / 'preview.wasm')
    # Runtime assets use relative URLs, including Worker imports, for a /repo/ Pages base.
    # Social metadata deliberately identifies the canonical public site.
    config = {
        'mode': 'remote' if args.compile_endpoint else 'static',
        'compileEndpoint': args.compile_endpoint,
        'precompiled': {'file': './preview.wasm', 'sha256': digest(output / 'preview.wasm'),
                        'sourceSha256': digest(output / 'ContentView.swift')},
    }
    (output / 'config.json').write_text(json.dumps(config, indent=2) + '\n')
    (output / 'THIRD_PARTY_NOTICES.txt').write_text(notices())
    (output / '.nojekyll').write_text('')
    (output / 'skills').mkdir()
    shutil.copy2(package_skill(), output / 'skills/ai-passport-local-compiler.zip')
    manifest = {p.relative_to(output).as_posix(): digest(p) for p in sorted(output.rglob('*')) if p.is_file()}
    (output / 'SHA256SUMS').write_text(''.join(f'{sha}  {name}\n' for name, sha in manifest.items()))
    archive = shutil.make_archive(str(output), 'zip', output)
    print(json.dumps({'output': str(output), 'zip': archive, 'mode': config['mode'],
                      'wasmBytes': (output / 'preview.wasm').stat().st_size}, indent=2))

if __name__ == '__main__': main()
