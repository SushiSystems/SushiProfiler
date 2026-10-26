# test_project.py
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""Tests the command lines the project service hands cmake, ctest and the checkers.

Every test runs against the `project` fixture, which records argv and starts no process.
"""

from __future__ import annotations

import sys
from pathlib import Path

import pytest

from sushiprofiler.services import project as project_service
from sushiprofiler.services.project import BuildType, Sanitizer, Size

pytestmark = pytest.mark.medium


def _is_configure(command: list[str]) -> bool:
    """Returns whether a recorded command is a cmake configure."""
    return "-S" in command


def _is_compile(command: list[str]) -> bool:
    """Returns whether a recorded command is a cmake build."""
    return "--build" in command


def _value_after(command: list[str], flag: str) -> str:
    """Returns the argument that follows *flag* in *command*."""
    return command[command.index(flag) + 1]


def test_build_configures_an_empty_tree_then_compiles(project):
    assert project_service.build(BuildType.RELEASE) == 0

    configure, compile_ = project.runner.commands
    assert _is_configure(configure) and _is_compile(compile_)
    assert "-DCMAKE_BUILD_TYPE=Release" in configure
    assert "-DSP_SANITIZE=" in configure
    assert "-DCMAKE_CXX_COMPILER=clang++" in configure


def test_build_leaves_a_configured_tree_alone(project):
    project.configure_tree("Debug")

    assert project_service.build() == 0

    assert [_is_compile(command) for command in project.runner.commands] == [True]


def test_build_with_a_define_reconfigures(project):
    project.configure_tree("Debug")

    assert project_service.build(defines=["FOO=1"]) == 0

    configure = project.runner.commands[0]
    assert _is_configure(configure) and _value_after(configure, "-D") == "FOO=1"


def test_build_with_a_sanitizer_uses_its_own_tree(project):
    assert project_service.build(BuildType.DEBUG, sanitizer=Sanitizer.THREAD) == 0

    configure = project.runner.commands[0]
    assert Path(_value_after(configure, "-B")).name == "build-tsan"
    assert "-DSP_SANITIZE=thread" in configure


def test_test_passes_the_sizes_the_jobs_and_the_filter_to_ctest(project):
    project.configure_tree("Debug")

    assert project_service.test(None, (Size.MEDIUM, Size.SMALL), filter="Small_.*", jobs=4) == 0

    ctest = project.runner.commands[-1]
    assert _value_after(ctest, "-L") == "^(medium|small)$"
    assert _value_after(ctest, "-R") == "Small_.*"
    assert _value_after(ctest, "--parallel") == "4"
    assert _value_after(ctest, "-LE") == "quarantined"
    assert _value_after(ctest, "-C") == "Debug"


def test_test_without_a_size_runs_every_size(project):
    project.configure_tree("Debug")

    assert project_service.test() == 0

    assert "-L" not in project.runner.commands[-1]


def test_test_with_package_installs_then_builds_and_runs_the_consumer(project):
    project.configure_tree("Debug")

    assert project_service.test(package=True) == 0

    install, configure, compile_, ctest = project.runner.commands[-4:]
    prefix = _value_after(install, "--prefix")
    assert "--install" in install and Path(prefix).name == "package-prefix"
    assert Path(_value_after(configure, "-S")) == project.root / "tests" / "package"
    assert f"-DCMAKE_PREFIX_PATH={prefix}" in configure
    assert _is_compile(compile_)
    assert Path(_value_after(ctest, "--test-dir")).name == "package"


def test_check_runs_every_checker_with_the_root(project):
    assert project_service.check() == 0

    assert len(project.runner.commands) == len(project_service.K_CHECKERS)
    assert all(command[0] == sys.executable and command[2] == str(project.root)
               for command in project.runner.commands)


def test_check_with_a_rule_runs_only_the_checkers_that_list_it(project, monkeypatch):
    monkeypatch.setattr(project_service, "rules_of",
                        lambda checker: frozenset({"rule_x"}) if "logs" in checker.parts
                        else frozenset())

    assert project_service.check(rule="rule_x") == 0

    (command,) = project.runner.commands
    assert command[1].endswith("check_logging.py") and command[-2:] == ["--rule", "rule_x"]


def test_check_with_an_unknown_rule_fails_and_says_so(project, monkeypatch):
    monkeypatch.setattr(project_service, "rules_of", lambda checker: frozenset())

    assert project_service.check(rule="rule_none") == 1

    assert project.console.of("error") == ["no checker under tools/ has the rule rule_none"]


def test_rules_of_reads_the_list_rules_flag(tmp_path):
    checker = tmp_path / "check_fake.py"
    checker.write_text(
        "import sys\nif sys.argv[1:] == ['--list-rules']:\n    print('rule_a\\nrule_b')\n",
        encoding="utf-8")

    assert project_service.rules_of(checker) == frozenset({"rule_a", "rule_b"})


def test_resolve_build_type_reads_the_tree_when_none_is_asked(project):
    project.configure_tree("RelWithDebInfo")

    assert project_service.resolve_build_type(None, project.tree) == BuildType.RELWITHDEBINFO
    assert project_service.resolve_build_type(BuildType.DEBUG, project.tree) == BuildType.DEBUG
