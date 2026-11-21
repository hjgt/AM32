#!/usr/bin/env python3
"""Build M1 with November 11 diagnostics and verify both published images."""
import hashlib
import json
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[2]
profile = root / "release/fd6288-g431"
args = json.loads((profile / "build-args.json").read_text())
# Preserve the historical NO_DIAGNOSTICS setting and disable the later PURE removal.
subprocess.run(["make", *args, "FD6288_M1_PURE=0"], cwd=root, check=True)
output = root / next(arg.removeprefix("OBJ=") for arg in args if arg.startswith("OBJ="))
expected = json.loads((profile / "SHA256.json").read_text())
for extension in ("bin", "hex"):
    published = f"AM32_CUSTOM_FD6288_G431_2.20_FINAL.{extension}"
    artifact = output / f"AM32_CUSTOM_FD6288_G431_2.20.{extension}"
    actual = hashlib.sha256(artifact.read_bytes()).hexdigest()
    if actual != expected[published]:
        raise SystemExit(f"Published M1 image mismatch: {artifact} ({actual})")
    print(f"Verified November 11 M1 {extension.upper()}: {actual}")
