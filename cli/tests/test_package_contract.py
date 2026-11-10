# test_package_contract.py
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""Tests the contract a consumer's package build runs `sp` under.

The contract is `build --build-dir --lean`, `package` and the compiler `SUSHISTACK_CXX` names.
"""

from __future__ import annotations

import re
from pathlib import Path

import pytest
from sushicore import deps_fragment
from typer.testing import CliRunner

from sushiprofiler import cli, config
from sushiprofiler.host_memory import MemoryHost
from sushiprofiler.services import project as project_service
from sushiprofiler.services.project import BuildType

pytestmark = pytest.mark.medium

_RUNNER = CliRunner()
_ANSI_ESCAPE = re.compile(r"\x1b\[[0-9;]*m")


def _value_after(command: list[str], flag: str) -> str:
    """Returns the argument that follows *flag* in *command*."""
    return command[command.index(flag) + 1]


def test_a_lean_build_configures_the_named_tree_without_tests_applications_or_benchmarks(project):
    assert project_service.build(BuildType.RELEASE, lean=True, tree_path="build/pkg-x",
                                 workspace=project.workspace) == 0

    configure = project.commands[0]
    assert Path(_value_after(configure, "-B")) == project.root / "build" / "pkg-x"
    for switch in ("SP_BUILD_TESTS", "SP_BUILD_APPLICATIONS", "SP_BUILD_BENCHMARKS"):
        assert f"-D{switch}=OFF" in configure


def test_a_default_build_asks_for_the_tests_and_the_applications(project):
    assert project_service.build(BuildType.RELEASE, workspace=project.workspace) == 0

    configure = project.commands[0]
    assert "-DSP_BUILD_TESTS=ON" in configure and "-DSP_BUILD_APPLICATIONS=ON" in configure
    assert "-DSP_BUILD_BENCHMARKS=OFF" not in configure


def test_a_lean_build_reconfigures_a_tree_that_has_tests(project):
    project.configure_tree("Release", tests="ON")

    assert project_service.build(BuildType.RELEASE, lean=True, workspace=project.workspace) == 0

    assert "-S" in project.commands[0]


def test_lean_refuses_the_benchmarks(project):
    assert project_service.build(BuildType.RELEASE, lean=True, benchmarks=True,
                                 workspace=project.workspace) == 2

    assert project.commands == []
    assert project.console.texts("error")


def test_the_package_command_is_registered_with_its_options():
    result = _RUNNER.invoke(cli.app, ["package", "--help"])
    text = _ANSI_ESCAPE.sub("", result.output)

    assert result.exit_code == 0
    for option in ("--build-dir", "--prefix", "--no-archive", "--force"):
        assert option in text


def test_load_config_reads_the_compiler_the_environment_names():
    host = MemoryHost(variables={config.K_STACK_CXX_ENV: "/stack/clang++"})

    assert config.load_config(host).stack_cxx == "/stack/clang++"


def test_the_fragment_names_the_cli():
    fragment = deps_fragment.read(Path(__file__).resolve().parents[1] / "sushistack.deps.toml")

    assert fragment.cli == config.PROFILE.program
