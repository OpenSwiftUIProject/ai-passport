#!/usr/bin/env python3
"""Package the standalone setup skill for direct download from GitHub Pages."""
from pathlib import Path
import zipfile

ROOT = Path(__file__).resolve().parent
NAME = 'ai-passport-local-compiler'


def package_skill():
    source = ROOT.parents[1] / 'skills' / NAME
    destination = ROOT / 'build' / f'{NAME}.zip'
    destination.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(destination, 'w', compression=zipfile.ZIP_DEFLATED) as archive:
        for path in sorted(source.rglob('*')):
            if path.is_file():
                archive.write(path, Path(NAME) / path.relative_to(source))
    return destination


if __name__ == '__main__': print(package_skill())
