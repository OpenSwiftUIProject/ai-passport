#!/usr/bin/env python3
"""Compare three WASM frames to the existing native LVGL host preview."""
from pathlib import Path
import json
import sys
import build

sys.path.insert(0, str(build.PASSPORT / 'tools'))
from capture_screen import Frame, save_png

baseline = Path(sys.argv[1]) if len(sys.argv) > 1 else build.WORKSPACE / 'work/openswiftui-preview/frame.fps1'
results = {}
for name, suffix in [('initial', ''), ('down', '.down'), ('hidden', '.hidden')]:
    frame = Frame('12345678')
    for line in baseline.with_name(baseline.name + suffix).read_bytes().splitlines():
        frame.accept(line)
    actual = (build.BUILD / f'{name}.rgb565').read_bytes()
    assert len(actual) == len(frame.pixels) == 153600
    differences = sum(actual[i:i + 2] != frame.pixels[i:i + 2] for i in range(0, len(actual), 2))
    results[name] = {'pixelsCompared': 76800, 'differentPixels': differences}
    assert differences == 0, (name, differences)
    save_png(frame, build.BUILD / f'{name}.png')
(build.BUILD / 'pixel-comparison.json').write_text(json.dumps(results, indent=2) + '\n')
print(json.dumps(results))
