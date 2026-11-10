# test_console.py
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""Tests that the console reads nothing until a printer is used, that sushiprofiler wires a lazy one, and that help needs no checkout."""

from __future__ import annotations

from pathlib import Path

import pytest
from sushicore.cli_console import LazyConsole
from typer.testing import CliRunner

from sushiprofiler import cli, console


def _recording_resolver(asked: list[str]):
    """Returns a config-folder resolver that notes each call in *asked*."""
    def resolve() -> Path:
        asked.append("asked")
        return Path("unused")
    return resolve


@pytest.mark.small
class TestALazyConsoleNotYetUsed:
    """Covers the `LazyConsole` that `sushiprofiler.console` wires, on its public surface."""

    def test_a_console_not_yet_used_is_not_built(self):
        assert not LazyConsole(lambda: Path("unused")).built

    def test_a_console_not_yet_used_has_not_asked_for_the_config_folder(self):
        asked: list[str] = []

        LazyConsole(_recording_resolver(asked))

        assert asked == []


@pytest.mark.small
class TestTheConsoleSushiprofilerWires:
    """Covers the console that `sushiprofiler.console` wires, through its accessor."""

    def test_sushiprofiler_wires_a_lazy_console(self):
        assert isinstance(console.wired(), LazyConsole)


@pytest.mark.medium
class TestHelpOutsideACheckout:
    """Covers the help command in a folder that is no checkout; it changes the working folder."""

    def test_help_works_outside_a_checkout(self, tmp_path, monkeypatch):
        monkeypatch.chdir(tmp_path)

        result = CliRunner().invoke(cli.app, ["--help"])

        assert result.exit_code == 0
