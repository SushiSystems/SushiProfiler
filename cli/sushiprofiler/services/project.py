# project.py
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""Drives SushiProfiler's build, test, bench, record and check steps.

Holds the build policy: the build tree of each sanitizer and build type, the memory gate every
configure installs, the test sizes and their CTest labels, the benchmark programs and sp_record,
the install the package test builds against, and the checkers. cmake and ctest run through
``sushicore.cmake_driver.CMakeDriver`` under the environment ``sushiprofiler.env`` assembles.
"""

from __future__ import annotations

import enum
import subprocess
import sys
from collections.abc import Mapping, Sequence
from dataclasses import dataclass
from functools import cached_property
from pathlib import Path
from typing import Protocol

from sushicore import build_admission, build_report, cmake_cache
from sushicore.build_tree import K_DEFAULT_BUILD_TYPE, refuse_another_build_type, tree_dir
from sushicore.cmake_driver import CMakeDriver
from sushicore.proc import CommandRunner, ConsoleLike, Runner
from sushicore.testing import ModuleTests, TestLabel, run_tests, select_labels
from sushicore.toolchain_args import c_compiler_for

from .. import console
from ..config import PROFILE, Config, find_project_root, load_config
from ..env import load_build_env


class BuildType(str, enum.Enum):
    """Names the build types `--type` accepts."""

    DEBUG = "debug"
    RELEASE = "release"
    RELWITHDEBINFO = "relwithdebinfo"


class Sanitizer(str, enum.Enum):
    """Names the sanitizer builds `--sanitize` accepts; each is an SP_SANITIZE value."""

    ADDRESS = "address"
    THREAD = "thread"


K_CMAKE_BUILD_TYPE = {
    BuildType.DEBUG: "Debug",
    BuildType.RELEASE: "Release",
    BuildType.RELWITHDEBINFO: "RelWithDebInfo",
}

#: The name of the tree of each sanitizer; ``tree_dir`` appends the build type.
K_TREE_NAME = {None: "default", Sanitizer.ADDRESS: "asan", Sanitizer.THREAD: "tsan"}

#: The type a command builds when it is not told one, and the type a benchmark runs in.
K_DEFAULT_TYPE = BuildType(K_DEFAULT_BUILD_TYPE.lower())
K_BENCHMARK_TYPE = BuildType.RELEASE

K_PACKAGE_SOURCE = Path("tests", "package")

K_BENCHMARKS_VARIABLE = "SP_BUILD_BENCHMARKS"
K_TESTS_VARIABLE = "SP_BUILD_TESTS"
K_APPLICATIONS_VARIABLE = "SP_BUILD_APPLICATIONS"
K_BENCHMARK_FOLDER = "benchmarks"
K_BENCHMARK_PREFIX = "sp_bench_"
K_APPLICATION_FOLDER = "applications"
K_RECORD_PROGRAM = "sp_record"

K_CHECKERS = (
    Path("tools", "documentation", "check_source_comments.py"),
    Path("tools", "documentation", "check_docs_layout.py"),
    Path("tools", "documentation", "check_changelog.py"),
    Path("tools", "documentation", "check_docs_truth.py"),
    Path("tools", "layering", "check_layering.py"),
    Path("tools", "testing", "check_test_mirror.py"),
    Path("tools", "testing", "check_test_sizes.py"),
    Path("tools", "testing", "check_probes.py"),
    Path("tools", "logs", "check_logging.py"),
)


class ServiceConsole(ConsoleLike, Protocol):
    """The console methods the project service prints through, beyond what the runner uses."""

    def info(self, text: str) -> None:
        """Prints an information line."""

    def header(self, title: str) -> None:
        """Prints a section header."""


@dataclass(frozen=True)
class Workspace:
    """Holds what the project service acts on and acts through.

    Attributes:
        root: The checkout's root folder.
        config: The resolved configuration.
        console: Where the service prints.
        runner: What starts cmake, ctest, the checkers and the programs.
        environment: The environment the started commands run under; ``None`` assembles it per
            tree through ``sushiprofiler.env.load_build_env``.
    """

    root: Path
    config: Config
    console: ServiceConsole
    runner: CommandRunner
    environment: Mapping[str, str] | None = None

    @cached_property
    def driver(self) -> CMakeDriver:
        """Returns the cmake and ctest driver over this workspace's console and runner."""
        return CMakeDriver(self.console, self.runner)

    def build_env(self, tree: Path) -> dict[str, str]:
        """Returns the environment cmake, ctest and the programs run under for *tree*."""
        if self.environment is not None:
            return dict(self.environment)
        return load_build_env(self.config, tree)


def current_workspace() -> Workspace:
    """Returns the workspace of the checkout `sp` runs in, with the real console and runner."""
    return Workspace(find_project_root(), load_config(), console,
                     Runner(console, PROFILE.program))


def build_dir(root: Path, sanitizer: Sanitizer | None, build_type: BuildType,
              requested: str | None = None) -> Path:
    """Returns the build tree: *requested* below *root* when given, else the one of *sanitizer*.

    The default tree is ``build/<name>-<type>``, so each build type has a tree of its own.
    """
    if requested:
        return root / requested
    return tree_dir(root, K_TREE_NAME[sanitizer], K_CMAKE_BUILD_TYPE[build_type])


def tree_switches(lean: bool) -> dict[str, str]:
    """Returns the cache variables of the tree's contents and their values for *lean* or not.

    The tests and the applications are on unless *lean*. A lean build also turns the
    benchmarks off; a plain build leaves them to ``--benchmarks``.
    """
    state = "OFF" if lean else "ON"
    switches = {K_TESTS_VARIABLE: state, K_APPLICATIONS_VARIABLE: state}
    if lean:
        switches[K_BENCHMARKS_VARIABLE] = "OFF"
    return switches


def resolve_build_type(requested: BuildType | None,
                       default: BuildType = K_DEFAULT_TYPE) -> BuildType:
    """Returns *requested*, else *default*; the type picks the tree, so no tree is asked."""
    return default if requested is None else requested


def benchmarks_defines(benchmarks: bool | None) -> list[str]:
    """Returns the cache variable that switches the benchmark programs on or off, as `VAR=VALUE`.

    Args:
        benchmarks: True builds them, False stops building them, None leaves the cached value.
    """
    if benchmarks is None:
        return []
    return [f"{K_BENCHMARKS_VARIABLE}={'ON' if benchmarks else 'OFF'}"]


def benchmark_programs(folder: Path) -> list[Path]:
    """Returns the benchmark programs in *folder*, sorted by name."""
    if not folder.is_dir():
        return []
    return sorted(path for path in folder.iterdir()
                  if path.is_file() and path.name.startswith(K_BENCHMARK_PREFIX)
                  and path.suffix in ("", ".exe"))


def _compiler_arguments(cfg: Config) -> list[str]:
    """Returns the generator and compiler arguments of a configure."""
    arguments: list[str] = []
    generator = cfg.resolved_generator()
    if generator:
        arguments += ["-G", generator]
    if generator == "Ninja":
        arguments += [f"-DCMAKE_MAKE_PROGRAM={cfg.resolved_ninja()}"]
    compiler = cfg.resolved_compiler()
    if compiler:
        arguments += [f"-DCMAKE_CXX_COMPILER={compiler}",
                      f"-DCMAKE_C_COMPILER={c_compiler_for(compiler)}"]
    return arguments


def _gate_arguments(cfg: Config, tree: Path) -> list[str]:
    """Returns the `-D` arguments that run every compile and link of *tree* through the gate."""
    cache = build_admission.compiler_cache(cfg.ccache_exe)
    return [f"-D{entry}" for entry in build_admission.launcher_defines(tree, cache)]


def _configure(space: Workspace, tree: Path, cmake_type: str, command: list[str],
               defines: Sequence[str], env: dict[str, str]) -> int:
    """Configures *tree* with *command* and the gate, once the tree is known to be its own.

    Raises:
        sushicore.errors.ConfigError: *tree* is configured as another build type than *cmake_type*.
        sushicore.build_admission.BuildStoppedError: the owner stopped builds of *tree*.
    """
    refuse_another_build_type(tree, cmake_type, defines)
    build_admission.refuse_a_stopped_gate(tree)
    return space.driver.configure([*command, *_gate_arguments(space.config, tree)],
                                  cwd=space.root, env=env)


def _compile(space: Workspace, tree: Path, env: dict[str, str], config: str) -> int:
    """Compiles *tree* at the hardware thread count; the gate holds each edge back until it fits."""
    return space.driver.compile(space.config, tree, cwd=space.root, env=env, config=config,
                                jobs=build_admission.hardware_threads())


def _needs_configure(space: Workspace, tree: Path, build_type: BuildType,
                     sanitizer: Sanitizer | None, switches: dict[str, str]) -> bool:
    """Returns whether *tree* must be configured before it is compiled."""
    cfg = space.config
    if not (tree / "CMakeCache.txt").is_file():
        return True
    expect = {"SP_SANITIZE": sanitizer.value if sanitizer else "", **switches}
    if not cfg.is_multi_config():
        expect["CMAKE_BUILD_TYPE"] = K_CMAKE_BUILD_TYPE[build_type]
    generator = cfg.resolved_generator()
    if generator:
        return space.driver.needs_configure(tree, generator, expect=expect)
    return any(cmake_cache.cached_value(tree, key) != value for key, value in expect.items())


def build(build_type: BuildType | None = None, clean: bool = False,
          defines: Sequence[str] = (), sanitizer: Sanitizer | None = None,
          benchmarks: bool | None = None, lean: bool = False,
          tree_path: str | None = None, report: bool = False,
          workspace: Workspace | None = None) -> int:
    """Configures the tree of *sanitizer* and *build_type* when it needs it or *defines* are
    given, then compiles it.

    Args:
        clean: Removes the tree first.
        defines: `VAR=VALUE` cache variables; any of them reconfigures a configured tree.
        build_type: The type to build; debug when None. Each type has its own tree.
        sanitizer: Builds in that sanitizer's own tree, build/asan-<type>/ or build/tsan-<type>/.
        benchmarks: True builds the benchmark programs, False stops building them, None keeps
            what the tree has.
        lean: Builds no tests, applications or benchmarks; what a library package is built as.
        tree_path: The tree to build in, below the project root, in place of the default one.
        report: Measures the build, then prints its report and writes it into the tree.
        workspace: The checkout to build; none builds the one `sp` runs in.

    Returns:
        The exit code; 2 when *lean* and ``benchmarks=True`` ask for opposite things.
    """
    space = workspace or current_workspace()
    if lean and benchmarks:
        space.console.error("--lean builds no benchmark programs, so --benchmarks cannot join it")
        return 2
    defines = [*defines, *benchmarks_defines(benchmarks)]
    switches = tree_switches(lean)
    root, cfg = space.root, space.config
    build_type = resolve_build_type(build_type)
    tree = build_dir(root, sanitizer, build_type, tree_path)
    if clean:
        space.driver.clean_tree(tree)
    env = space.build_env(tree)

    if defines or _needs_configure(space, tree, build_type, sanitizer, switches):
        command = [space.driver.cmake(cfg), "-S", str(root), "-B", str(tree),
                   f"-DCMAKE_BUILD_TYPE={K_CMAKE_BUILD_TYPE[build_type]}",
                   f"-DSP_SANITIZE={sanitizer.value if sanitizer else ''}",
                   *(f"-D{name}={value}" for name, value in switches.items()),
                   *_compiler_arguments(cfg)]
        for define in defines:
            command += ["-D", define]
        rc = _configure(space, tree, K_CMAKE_BUILD_TYPE[build_type], command, defines, env)
        if rc != 0:
            return rc

    offset = build_report.ninja_log_offset(tree)
    rc = _compile(space, tree, env, K_CMAKE_BUILD_TYPE[build_type])
    if report:
        _print_report(space, tree, offset)
    return rc


def _print_report(space: Workspace, tree: Path, offset: int) -> None:
    """Prints the report of the build that appended to the log of *tree* from *offset*, and
    writes it into *tree*."""
    measured = build_report.of_build(tree, offset, build_admission.state_dir(tree))
    for line in measured.lines():
        space.console.info(line)
    space.console.info(f"Report: {measured.write(tree)}")


def test(build_type: BuildType | None = None, sizes: Sequence[TestLabel] = (), clean: bool = False,
         filter: str | None = None, sanitizer: Sanitizer | None = None,
         jobs: int | None = None, package: bool = False, repeat: int = 0,
         every_size: bool = False, workspace: Workspace | None = None) -> int:
    """Builds when needed, then runs the tests *sizes* and *filter* select through the shared test command.

    Args:
        sizes: The sizes to run; none runs small and medium.
        filter: A ctest -R regex over the `Suite.Case` names.
        sanitizer: Builds and tests that sanitizer's tree.
        jobs: Tests run at once; none leaves the count to ctest.
        package: Also installs the tree, builds tests/package/ against the install and runs
            its test.
        repeat: Reruns each test up to this many times, stopping at its first failure.
        every_size: Runs every size, the large tests included.
        workspace: The checkout to test; none tests the one `sp` runs in.
    """
    space = workspace or current_workspace()
    build_type = resolve_build_type(build_type)
    rc = build(build_type, clean=clean, sanitizer=sanitizer, workspace=space)
    if rc != 0:
        return rc

    root, cfg, driver = space.root, space.config, space.driver
    tree = build_dir(root, sanitizer, build_type)
    config = K_CMAKE_BUILD_TYPE[build_type]
    env = space.build_env(tree)
    module = ModuleTests(
        profile=PROFILE, project_root=lambda: root, console=lambda: space.console,
        build_tree=lambda: tree, run=space.runner.run,
        run_ctest=lambda directory, **options: driver.ctest_run(cfg, directory, env, config=config, **options))
    selection = select_labels(sizes, every_size=every_size)
    rc = run_tests(module, selection, name_filter=filter, repeat=repeat, jobs=jobs, every_size=every_size)
    if rc != 0 or not package:
        return rc
    return _test_package(space, tree, env, config)


def _test_package(space: Workspace, tree: Path, env: dict[str, str], config: str) -> int:
    """Installs *tree*, then builds and runs the consumer of tests/package/ against the install."""
    root, cfg, driver = space.root, space.config, space.driver
    prefix = tree / "package-prefix"
    consumer = tree / "package"
    space.console.header("Package consumer")
    rc = space.runner.run([driver.cmake(cfg), "--install", str(tree), "--config", config,
                      "--prefix", str(prefix)], root, env)
    if rc != 0:
        return rc
    rc = _configure(space, consumer, config,
                    [driver.cmake(cfg), "-S", str(root / K_PACKAGE_SOURCE),
                     "-B", str(consumer), f"-DCMAKE_BUILD_TYPE={config}",
                     f"-DCMAKE_PREFIX_PATH={prefix}", *_compiler_arguments(cfg)],
                    (), env)
    if rc != 0:
        return rc
    rc = _compile(space, consumer, env, config)
    if rc != 0:
        return rc
    return driver.ctest_run(cfg, consumer, env, config=config)


def bench(build_type: BuildType | None = None, workspace: Workspace | None = None) -> int:
    """Runs every built benchmark program of the plain tree in name order.

    Args:
        build_type: The build whose programs run; release when None, the type that ships.
        workspace: The checkout to run in; none uses the one `sp` runs in.

    Returns:
        The first non-zero exit code among the programs, or 1 when the tree has none.
    """
    space = workspace or current_workspace()
    root = space.root
    build_type = resolve_build_type(build_type, K_BENCHMARK_TYPE)
    tree = build_dir(root, None, build_type)
    config = K_CMAKE_BUILD_TYPE[build_type]
    programs = benchmark_programs(tree / K_BENCHMARK_FOLDER / config)
    if not programs:
        space.console.error(f"no benchmark program in {tree / K_BENCHMARK_FOLDER / config}; "
                      f"run {PROFILE.program} build --benchmarks --type release first")
        return 1
    env = space.build_env(tree)
    code = 0
    for program in programs:
        space.console.header(program.name)
        code = code or space.runner.run([str(program)], root, env)
    return code


def record_program(folder: Path) -> Path | None:
    """Returns the sp_record program in *folder*, or None when the build has not made it."""
    for name in (f"{K_RECORD_PROGRAM}.exe", K_RECORD_PROGRAM):
        if (folder / name).is_file():
            return folder / name
    return None


def record(arguments: Sequence[str], build_type: BuildType | None = None,
           workspace: Workspace | None = None) -> int:
    """Runs sp_record of the plain tree on *arguments*, from the current folder.

    Args:
        arguments: The command, then its arguments, as sp_record takes them.
        build_type: The build whose program runs; debug when None.
        workspace: The checkout to run in; none uses the one `sp` runs in.

    Returns:
        sp_record's exit code, or 1 when the tree holds no sp_record.
    """
    space = workspace or current_workspace()
    build_type = resolve_build_type(build_type)
    tree = build_dir(space.root, None, build_type)
    folder = tree / K_APPLICATION_FOLDER / K_CMAKE_BUILD_TYPE[build_type]
    program = record_program(folder)
    if program is None:
        space.console.error(f"no {K_RECORD_PROGRAM} in {folder}; run {PROFILE.program} build first")
        return 1
    return space.runner.run([str(program), *arguments], Path.cwd(), space.build_env(tree))


def check(report: bool = False, rule: str | None = None,
          workspace: Workspace | None = None) -> int:
    """Runs the shared checkers under tools/.

    Args:
        report: Makes each checker print counts per rule and exit zero.
        rule: The one rule to run; only the checkers whose --list-rules names it run.
        workspace: The checkout to check; none checks the one `sp` runs in.

    Returns:
        The highest exit code among the checkers that ran, or 1 when no checker has *rule*.
    """
    space = workspace or current_workspace()
    root = space.root
    checkers = [checker for checker in K_CHECKERS
                if rule is None or rule in rules_of(root / checker)]
    if not checkers:
        space.console.error(f"no checker under tools/ has the rule {rule}")
        return 1
    arguments = (["--report"] if report else []) + (["--rule", rule] if rule else [])
    return max(space.runner.run([sys.executable, str(root / checker), str(root), *arguments], root)
               for checker in checkers)


def rules_of(checker: Path) -> frozenset[str]:
    """Returns the rule names the checker at *checker* prints for --list-rules."""
    listed = subprocess.run([sys.executable, str(checker), "--list-rules"],
                            capture_output=True, text=True, check=False)
    return frozenset(listed.stdout.split()) if listed.returncode == 0 else frozenset()
