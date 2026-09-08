#!/usr/bin/env python3
"""Package the already verified, playable 2048 build. Never flashes or uploads."""
from pathlib import Path
import hashlib
import json
import os
import shutil
import subprocess
import sys
import zipfile


def git(root, *args):
    return subprocess.check_output(["git", "-C", str(root), *args], text=True).strip()


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    root = Path(__file__).resolve().parents[1]
    build = Path(sys.argv[1]).resolve()
    osui = Path(os.environ.get("OPENSWIFTUI_SOURCE_DIR", root.parent / "framework/OpenSwiftUI")).resolve()
    cache = (build / "CMakeCache.txt").read_text()
    if "PASSPORT_BOOT_2048:BOOL=ON" not in cache or "PASSPORT_2048_SOAK:BOOL=OFF" not in cache:
        raise SystemExit("Release requires BOOT_2048=ON and SOAK=OFF")
    subprocess.run([sys.executable, str(root / "tools/verify_firmware.py"), str(build)], check=True)
    revision = git(root, "rev-parse", "HEAD")
    if git(root, "status", "--porcelain") or git(osui, "status", "--porcelain"):
        raise SystemExit("Commit reviewed source changes before packaging an identifiable release")
    # Immutable candidate folder: never overwrite an earlier bundle or evidence.
    output = root.parent / "work/releases" / ("pocket-2048-" + revision[:12])
    output.mkdir(parents=True, exist_ok=False)
    bundle = output / "bundle"
    bundle.mkdir()
    prefix = "pocket-2048-" + revision[:12]
    for source, suffix in [("FoloToy-AI-Passport-full.bin", "full.bin"), ("FoloToy-AI-Passport.bin", "app.bin")]:
        shutil.copy2(build / source, bundle / (prefix + "-" + suffix))
    for source, target in [("docs/development/release/pocket-2048.md", "README.md"),
                           ("docs/development/release/pocket-2048.zh_CN.md", "README.zh_CN.md"),
                           ("LICENSE", "LICENSE")]:
        shutil.copy2(root / source, bundle / target)
    for name in ("README.md", "README.zh_CN.md"):
        doc = bundle / name
        doc.write_text(doc.read_text().replace("pocket-2048.zh_CN.md", "README.zh_CN.md").replace("pocket-2048.md", "README.md"))
    shutil.copy2(osui / "LICENSE", bundle / "LICENSE-OpenSwiftUI")
    manifest = {
        "application": "Pocket 2048", "candidate": prefix,
        "source": {"url": "https://github.com/OpenSwiftUIProject/ai-passport", "revision": revision},
        "openswiftui": {"url": "https://github.com/OpenSwiftUIProject/OpenSwiftUI", "revision": git(osui, "rev-parse", "HEAD")},
        "build": {"idf": "5.5.3", "swift": subprocess.check_output(["swiftc", "--version"], text=True).strip(),
                  "boot2048": True, "soak": False, "sdkconfigSHA256": sha(build / "sdkconfig"),
                  "dependenciesLockSHA256": sha(root / "dependencies.lock")},
        "installation": {"communityImage": prefix + "-full.bin", "developerAppImage": prefix + "-app.bin",
                         "appOffset": "0x10000", "appLimit": "0x300000", "identityAndRecoveryIncluded": False},
        "verification": {"build": "PASS", "hostTests": "PASS", "deviceTests": "NOT RUN",
                         "communityInstallation": "NOT RUN"},
        "files": {p.name: {"bytes": p.stat().st_size, "sha256": sha(p)} for p in sorted(bundle.glob("*.bin"))},
    }
    (bundle / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    (bundle / "SHA256SUMS").write_text("".join(f"{sha(p)}  {p.name}\n" for p in sorted(bundle.iterdir()) if p.is_file()))
    archive = output / (prefix + ".zip")
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as zip_file:
        for path in sorted(bundle.iterdir()):
            zip_file.write(path, prefix + "/" + path.name)
    (output / "SHA256SUMS").write_text(f"{sha(archive)}  {archive.name}\n")
    # Keep exact debug symbols and build configuration privately, outside the ZIP.
    debug = output / "debug"
    debug.mkdir()
    for name in ("FoloToy-AI-Passport.elf", "FoloToy-AI-Passport.map", "sdkconfig", "CMakeCache.txt", "flash_args"):
        shutil.copy2(build / name, debug / name)
    print("2048 release candidate:", output)
    print("Archive SHA-256:", sha(archive))


if __name__ == "__main__":
    main()
