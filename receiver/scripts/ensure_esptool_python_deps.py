"""PlatformIO pre-build repair for esptool Python dependencies.

Espressif's esptool imports intelhex and rich_click. Some PlatformIO
tool-esptoolpy installations omit one or both from PlatformIO's own
Python virtual environment. This script runs inside that environment
before the build and installs only missing modules.
"""
from __future__ import annotations

import importlib.util
import subprocess
import sys

REQUIREMENTS = (
    ("intelhex", "intelhex==2.3.0"),
    ("rich_click", "rich-click<2"),
)

missing = [
    requirement
    for module_name, requirement in REQUIREMENTS
    if importlib.util.find_spec(module_name) is None
]

if missing:
    print("PlatformIO preflight: repairing esptool Python dependencies...")
    print("Installing: " + ", ".join(missing))
    subprocess.check_call(
        [
            sys.executable,
            "-m",
            "pip",
            "install",
            "--disable-pip-version-check",
            "--no-input",
            *missing,
        ]
    )
else:
    print("PlatformIO preflight: esptool Python dependencies are present.")
