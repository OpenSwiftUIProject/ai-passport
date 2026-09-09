#!/usr/bin/env python3
"""Download and verify pinned official WASI C libraries; Swift must be installed."""
import hashlib
from pathlib import Path
import subprocess
import urllib.request
import build

downloads = [
    ('wasi-sysroot-34.0', '9d813544eeebe38b7b8f2244ed591de46b6db812c6dd1a257ff9f0d2a905a2be',
     ['wasi-sysroot-34.0/include', 'wasi-sysroot-34.0/lib/wasm32-wasip1', 'wasi-sysroot-34.0/share']),
    ('libclang_rt-34.0', 'eee3e634dcf71aa22b1333391623cf5c9965a637dc428a27b1a858c026c587f1',
     ['libclang_rt-34.0/wasm32-unknown-wasip1']),
]
destination = build.WORKSPACE / 'toolchains'
destination.mkdir(exist_ok=True)
for name, expected, members in downloads:
    archive = destination / f'{name}.tar.gz'
    if not archive.exists():
        temporary = archive.with_suffix('.download')
        urllib.request.urlretrieve(f'https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-34/{archive.name}', temporary)
        if hashlib.sha256(temporary.read_bytes()).hexdigest() != expected:
            raise RuntimeError(f'Checksum mismatch: {temporary}')
        temporary.replace(archive)
    if hashlib.sha256(archive.read_bytes()).hexdigest() != expected:
        raise RuntimeError(f'Checksum mismatch: {archive}')
    subprocess.run(['tar', '-xzf', str(archive), '-C', str(destination)] + members, check=True)
    print(f'Verified {archive.name}')
