# test_main.py
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""Tests that `python -m sushiprofiler` runs the CLI."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

import pytest

pytestmark = pytest.mark.medium

K_CLI = Path(__file__).resolve().parents[1]


def test_the_module_entry_prints_the_help():
    result = subprocess.run([sys.executable, "-m", "sushiprofiler", "--help"], cwd=K_CLI,
                            capture_output=True, text=True, check=False, timeout=120)

    assert result.returncode == 0
    assert "build" in result.stdout and "check" in result.stdout
