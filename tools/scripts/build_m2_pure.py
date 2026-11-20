#!/usr/bin/env python3
"""Build the archived M2 profile. No programmer or hardware access."""
import json
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[2]
args = json.loads((root / 'docs/m2-startup/build-args-m2-pure.json').read_text())
subprocess.run(['make', *args], cwd=root, check=True)
