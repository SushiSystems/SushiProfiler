# test_cli.py
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""Tests that each `sp` command parses its options and hands them to the project service.

Each command runs against the `project` fixture's workspace, handed in as the context object, so
what a command passed on shows in the argv the workspace's runner recorded.
"""

from __future__ import annotations

import re
from pathlib import Path

import pytest
from typer.testing import CliRunner

from sushiprofiler import cli

pytestmark = pytest.mark.medium

_RUNNER = CliRunner()
_ANSI_ESCAPE = re.compile(r"\x1b\[[0-9;]*m")


def _value_after(command: list[str], flag: str) -> str:
    """Returns the argument that follows *flag* in *command*."""
    return command[command.index(flag) + 1]


def _invoke(project, *arguments: str):
    """Runs `sp` with *arguments* on the workspace of *project*."""
    return _RUNNER.invoke(cli.app, list(arguments), obj=project.workspace)


def _plant_program(folder: Path, name: str) -> None:
    """Writes an empty program file *name* into *folder*."""
    folder.mkdir(parents=True)
    (folder / name).write_text("", encoding="utf-8")


@pytest.mark.parametrize(
    "command", ["build", "test", "bench", "check", "record", "docs", "backlog", "design"])
def test_help_lists_each_command(project, command):
    result = _invoke(project, "--help")

    assert result.exit_code == 0
    assert command in result.output


def test_build_passes_type_sanitizer_and_defines(project):
    result = _invoke(project, "build", "--type", "release", "--sanitize", "address",
                     "-D", "A=1")

    assert result.exit_code == 0
    configure = project.commands[0]
    assert Path(_value_after(configure, "-B")).name == "asan-release"
    assert "-DSP_SANITIZE=address" in configure
    assert "-DCMAKE_BUILD_TYPE=Release" in configure
    assert _value_after(configure, "-D") == "A=1"


def test_build_without_a_type_builds_debug(project):
    result = _invoke(project, "build")

    assert result.exit_code == 0
    assert Path(_value_after(project.commands[0], "-B")).name == "default-debug"


def test_build_passes_the_report_flag(project):
    project.configure_tree("Debug")

    result = _invoke(project, "build", "--report")

    assert result.exit_code == 0
    assert any(line.startswith("Report: ") for line in project.console.texts("info"))


def test_build_passes_benchmarks(project):
    result = _invoke(project, "build", "--benchmarks")

    assert result.exit_code == 0
    assert "SP_BUILD_BENCHMARKS=ON" in project.commands[0]


def test_build_passes_the_tree_and_lean(project):
    result = _invoke(project, "build", "--build-dir", "build/pkg-x", "--lean")

    assert result.exit_code == 0
    configure = project.commands[0]
    assert Path(_value_after(configure, "-B")) == project.root / "build" / "pkg-x"
    assert "-DSP_BUILD_TESTS=OFF" in configure


def test_bench_passes_the_type(project):
    _plant_program(project.tree_of("Debug") / "benchmarks" / "Debug", "sp_bench_zones.exe")

    result = _invoke(project, "bench", "--type", "debug")

    assert result.exit_code == 0
    (command,) = project.commands
    assert Path(command[0]).name == "sp_bench_zones.exe"


def test_test_passes_labels_jobs_filter_and_package(project):
    project.configure_tree("Debug")

    result = _invoke(project, "test", "--label", "small", "--label", "medium",
                     "--jobs", "3", "--filter", "X.*", "--package")

    assert result.exit_code == 0
    ctest = next(command for command in project.commands if "-L" in command)
    assert _value_after(ctest, "-L") == "^(small|medium)$"
    assert _value_after(ctest, "--parallel") == "3"
    assert _value_after(ctest, "-R") == "X.*"
    assert any("--install" in command for command in project.commands)


def test_test_with_all_selects_every_size(project):
    project.configure_tree("Debug")

    result = _invoke(project, "test", "--all")

    assert result.exit_code == 0
    ctest = next(command for command in project.commands if "-L" in command)
    assert _value_after(ctest, "-L") == "^(small|medium|large)$"


def test_test_refuses_zero_jobs(project):
    result = _invoke(project, "test", "--jobs", "0")

    assert result.exit_code != 0
    assert project.commands == []


def test_check_passes_report_and_rule(project):
    checker = project.root / "tools" / "logs" / "check_logging.py"
    checker.parent.mkdir(parents=True)
    checker.write_text("import sys\nif sys.argv[1:] == ['--list-rules']:\n    print('rule_a')\n",
                       encoding="utf-8")

    result = _invoke(project, "check", "--report", "--rule", "rule_a")

    assert result.exit_code == 0
    (command,) = project.commands
    assert command[-3:] == ["--report", "--rule", "rule_a"]


def test_docs_bundle_asks_for_a_release(project):
    result = _invoke(project, "docs", "bundle")

    assert result.exit_code == 2
    assert "--release" in _ANSI_ESCAPE.sub("", result.output)


def test_a_failing_service_becomes_the_exit_code(project):
    result = _invoke(project, "check", "--rule", "rule_none")

    assert result.exit_code == 1
    assert project.console.texts("error") == ["no checker under tools/ has the rule rule_none"]


def test_record_show_passes_analyses_top_group_and_baseline(project):
    _plant_program(project.tree_of("Release") / "applications" / "Release", "sp_record.exe")

    result = _invoke(project, "record", "show", "run.json", "-a", "breakdown",
                     "--analysis", "top_spans", "--top", "5", "--group-by", "op",
                     "--baseline", "off.json", "--type", "release")

    assert result.exit_code == 0
    (command,) = project.commands
    assert command[1:] == ["show", "run.json", "--analysis", "breakdown", "--analysis",
                           "top_spans", "--group-by", "op", "--top", "5", "--baseline",
                           "off.json"]


def test_record_compare_passes_both_records_and_the_mismatch_flag(project):
    _plant_program(project.tree_of("Debug") / "applications" / "Debug", "sp_record.exe")

    result = _invoke(project, "record", "compare", "a.json", "b.json", "--allow-build-mismatch")

    assert result.exit_code == 0
    (command,) = project.commands
    assert command[1:] == ["compare", "a.json", "b.json", "--allow-build-mismatch"]


def test_record_check_passes_its_files(project):
    _plant_program(project.tree_of("Debug") / "applications" / "Debug", "sp_record.exe")

    result = _invoke(project, "record", "check", "run.json", "budgets.json")

    assert result.exit_code == 0
    (command,) = project.commands
    assert command[1:] == ["check", "run.json", "budgets.json"]


def test_record_validate_passes_its_file(project):
    _plant_program(project.tree_of("Debug") / "applications" / "Debug", "sp_record.exe")

    result = _invoke(project, "record", "validate", "run.json")

    assert result.exit_code == 0
    (command,) = project.commands
    assert command[1:] == ["validate", "run.json"]


def test_record_exits_with_the_programs_code(project):
    _plant_program(project.tree_of("Debug") / "applications" / "Debug", "sp_record.exe")
    project.runner.script(1)

    result = _invoke(project, "record", "validate", "run.json")

    assert result.exit_code == 1
