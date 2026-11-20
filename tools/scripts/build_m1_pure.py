#!/usr/bin/env python3
"""Rebuild M1 PURE from its archived profile. No hardware access."""
import json, subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[2]
subprocess.run(['make',*json.loads((root/'release/m1-pure/build-args.json').read_text())],cwd=root,check=True)
