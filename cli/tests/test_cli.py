# test_cli.py
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""Tests that each `sp` command parses its options and hands them to the project service."""

from __future__ import annotations

import re

import pytest
from typer.testing import CliRunner

from sushiprofiler import cli
from sushiprofiler.services.project import BuildType, Sanitizer, Size

pytestmark = pytest.mark.small

_RUNNER = CliRunner()
_ANSI_ESCAPE = re.compile(r"\x1b\[[0-9;]*m")


@pytest.fixture
def calls(monkeypatch) -> list[tuple[str, tuple, dict]]:
    """Returns the calls the commands make into the project service, which does nothing."""
    recorded: list[tuple[str, tuple, dict]] = []
    for name in ("build", "test", "check"):
        def record(*args, _name=name, **kwargs):
            recorded.append((_name, args, kwargs))
            return 0
        monkeypatch.setattr(cli.project_service, name, record)
    return recorded


def test_help_lists_the_three_commands():
    result = _RUNNER.invoke(cli.app, ["--help"])

    assert result.exit_code == 0
    for command in ("build", "test", "check"):
        assert command in result.output


def test_build_passes_type_sanitizer_and_defines(calls):
    result = _RUNNER.invoke(cli.app, ["build", "--type", "release", "--sanitize", "address",
                                      "-D", "A=1"])

    assert result.exit_code == 0
    (name, args, kwargs), = calls
    assert name == "build" and args == (BuildType.RELEASE,)
    assert kwargs == {"clean": False, "defines": ["A=1"], "sanitizer": Sanitizer.ADDRESS}


def test_test_passes_labels_jobs_filter_and_package(calls):
    result = _RUNNER.invoke(cli.app, ["test", "--label", "small", "--label", "medium",
                                      "--jobs", "3", "--filter", "X.*", "--package"])

    assert result.exit_code == 0
    (name, args, kwargs), = calls
    assert name == "test" and args == (None, [Size.SMALL, Size.MEDIUM])
    assert kwargs["jobs"] == 3 and kwargs["filter"] == "X.*" and kwargs["package"] is True


def test_test_refuses_zero_jobs(calls):
    result = _RUNNER.invoke(cli.app, ["test", "--jobs", "0"])

    assert result.exit_code != 0
    assert calls == []


def test_check_passes_report_and_rule(calls):
    result = _RUNNER.invoke(cli.app, ["check", "--report", "--rule", "rule_a"])

    assert result.exit_code == 0
    assert calls == [("check", (), {"report": True, "rule": "rule_a"})]


def test_help_lists_the_docs_group():
    result = _RUNNER.invoke(cli.app, ["--help"])

    assert result.exit_code == 0
    assert "docs" in result.output


def test_docs_bundle_asks_for_a_release():
    result = _RUNNER.invoke(cli.app, ["docs", "bundle"])

    assert result.exit_code == 2
    assert "--release" in _ANSI_ESCAPE.sub("", result.output)


def test_a_failing_service_becomes_the_exit_code(monkeypatch):
    monkeypatch.setattr(cli.project_service, "check", lambda **kwargs: 2)

    assert _RUNNER.invoke(cli.app, ["check"]).exit_code == 2
