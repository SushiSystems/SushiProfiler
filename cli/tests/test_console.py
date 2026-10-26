# test_console.py
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""Tests that importing the console reads nothing until a printer is used."""

from __future__ import annotations

import importlib

import pytest

pytestmark = pytest.mark.small


def test_importing_the_console_builds_nothing():
    console = importlib.reload(importlib.import_module("sushiprofiler.console"))

    assert not console._lazy.built


def test_help_works_outside_a_checkout(tmp_path, monkeypatch):
    from typer.testing import CliRunner

    from sushiprofiler import cli

    monkeypatch.chdir(tmp_path)

    result = CliRunner().invoke(cli.app, ["--help"])

    assert result.exit_code == 0
