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

from sushicore import build_admission
from sushicore.errors import ConfigError

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


def test_the_default_build_type_is_debug():
    assert project_service.K_DEFAULT_TYPE == BuildType.DEBUG
    assert project_service.resolve_build_type(None) == BuildType.DEBUG
    assert project_service.resolve_build_type(BuildType.RELEASE) == BuildType.RELEASE


def test_a_build_without_a_type_configures_the_debug_tree(project):
    assert project_service.build() == 0

    configure, compile_ = project.runner.commands
    assert "-DCMAKE_BUILD_TYPE=Debug" in configure
    assert Path(_value_after(configure, "-B")) == project.tree_of("Debug")
    assert Path(_value_after(compile_, "--build")) == project.tree_of("Debug")


@pytest.mark.parametrize("build_type, folder", [
    (BuildType.DEBUG, "default-debug"),
    (BuildType.RELEASE, "default-release"),
    (BuildType.RELWITHDEBINFO, "default-relwithdebinfo"),
])
def test_each_build_type_has_a_tree_of_its_own(project, build_type, folder):
    assert project_service.build(build_type) == 0

    assert Path(_value_after(project.runner.commands[0], "-B")) == project.root / "build" / folder


def test_a_configure_of_another_type_into_a_tree_is_refused(project):
    project.configure_tree("Debug")

    with pytest.raises(ConfigError, match="Debug tree"):
        project_service.build(BuildType.RELEASE, defines=["A=1"], tree_path="build/default-debug")

    assert project.runner.commands == []


def test_a_define_of_the_build_type_is_refused(project):
    with pytest.raises(ConfigError, match="CMAKE_BUILD_TYPE"):
        project_service.build(defines=["CMAKE_BUILD_TYPE=Release"])

    assert project.runner.commands == []


def test_a_stopped_gate_refuses_the_configure(project):
    stop = build_admission.stop_file(project.tree_of("Debug"))
    stop.parent.mkdir(parents=True)
    stop.write_text("", encoding="utf-8")

    with pytest.raises(build_admission.BuildStoppedError):
        project_service.build()

    assert project.runner.commands == []


def test_a_configure_carries_both_launcher_entries(project):
    assert project_service.build() == 0

    configure = project.runner.commands[0]
    entries = [part for part in configure if part.startswith("-DCMAKE_CXX_")]
    assert sorted(entry.split("=", 1)[0] for entry in entries) == [
        "-DCMAKE_CXX_COMPILER", "-DCMAKE_CXX_COMPILER_LAUNCHER", "-DCMAKE_CXX_LINKER_LAUNCHER"]
    assert all(str(build_admission.state_dir(project.tree_of("Debug")).as_posix()) in entry
               for entry in entries if "LAUNCHER" in entry)


def test_the_compile_runs_at_the_hardware_thread_count(project, monkeypatch):
    monkeypatch.setattr(build_admission, "hardware_threads", lambda: 7)

    assert project_service.build() == 0

    assert _value_after(project.runner.commands[-1], "--parallel") == "7"


def test_a_build_with_a_report_prints_it_and_writes_it_into_the_tree(project):
    tree = project.configure_tree("Debug")

    assert project_service.build(report=True) == 0

    assert any(line.startswith("Report: ") for line in project.console.of("info"))
    assert (tree / "build_report.json").is_file()


def test_a_build_without_a_report_prints_none(project):
    tree = project.configure_tree("Debug")

    assert project_service.build() == 0

    assert not (tree / "build_report.json").exists()


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
    assert Path(_value_after(configure, "-B")) == project.tree_of("Debug", "tsan")
    assert "-DSP_SANITIZE=thread" in configure


def test_build_with_benchmarks_switches_them_on_and_reconfigures(project):
    project.configure_tree("Release")

    assert project_service.build(BuildType.RELEASE, benchmarks=True) == 0

    configure = project.runner.commands[0]
    assert _is_configure(configure) and _value_after(configure, "-D") == "SP_BUILD_BENCHMARKS=ON"


def test_build_without_benchmarks_switches_them_off(project):
    project.configure_tree("Release")

    assert project_service.build(BuildType.RELEASE, benchmarks=False) == 0

    assert _value_after(project.runner.commands[0], "-D") == "SP_BUILD_BENCHMARKS=OFF"


def test_bench_runs_every_benchmark_program_in_name_order(project):
    project.configure_tree("Release")
    folder = project.tree_of("Release") / "benchmarks" / "Release"
    folder.mkdir(parents=True)
    for name in ("sp_bench_zones.exe", "sp_bench_clock.exe", "sp_zones_tests.exe"):
        (folder / name).write_text("", encoding="utf-8")

    assert project_service.bench() == 0

    assert [Path(command[0]).name for command in project.runner.commands] == [
        "sp_bench_clock.exe", "sp_bench_zones.exe"]


def test_bench_without_a_benchmark_program_fails_and_says_so(project):
    assert project_service.bench() == 1

    (message,) = project.console.of("error")
    assert "sp build --benchmarks --type release" in message and project.runner.commands == []


def test_bench_looks_in_the_release_tree_unless_told_another(project):
    folder = project.configure_tree("Debug") / "benchmarks" / "Debug"
    folder.mkdir(parents=True)
    (folder / "sp_bench_zones.exe").write_text("", encoding="utf-8")

    assert project_service.bench() == 1
    assert project_service.bench(BuildType.DEBUG) == 0


def test_record_runs_sp_record_of_the_asked_type_with_the_arguments(project):
    folder = project.configure_tree("Release") / "applications" / "Release"
    folder.mkdir(parents=True)
    (folder / "sp_record.exe").write_text("", encoding="utf-8")

    assert project_service.record(["validate", "run.json"], BuildType.RELEASE) == 0

    (command,) = project.runner.commands
    assert Path(command[0]).name == "sp_record.exe" and command[1:] == ["validate", "run.json"]


def test_record_defaults_to_the_debug_tree(project):
    folder = project.configure_tree("Debug") / "applications" / "Debug"
    folder.mkdir(parents=True)
    (folder / "sp_record.exe").write_text("", encoding="utf-8")

    assert project_service.record(["validate", "run.json"]) == 0


def test_record_without_the_program_fails_and_says_so(project):
    assert project_service.record(["validate", "run.json"]) == 1

    (message,) = project.console.of("error")
    assert "sp build" in message and project.runner.commands == []


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
    assert any(part.startswith("-DCMAKE_CXX_COMPILER_LAUNCHER=") for part in configure)


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


def test_resolve_build_type_ignores_the_type_a_tree_carries(project):
    project.configure_tree("RelWithDebInfo")

    assert project_service.resolve_build_type(None) == BuildType.DEBUG
