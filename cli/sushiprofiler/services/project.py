# project.py
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
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
from collections.abc import Sequence
from pathlib import Path

from sushicore import build_admission, build_report, cmake_cache
from sushicore.build_tree import K_DEFAULT_BUILD_TYPE, refuse_another_build_type, tree_dir
from sushicore.cmake_driver import CMakeDriver
from sushicore.proc import Runner
from sushicore.toolchain_args import c_compiler_for

from .. import console
from ..config import PROFILE, Config, find_project_root, load_config
from ..env import load_build_env


class BuildType(str, enum.Enum):
    """Names the build types `--type` accepts."""

    DEBUG = "debug"
    RELEASE = "release"
    RELWITHDEBINFO = "relwithdebinfo"


class Size(str, enum.Enum):
    """Names the test sizes `--label` accepts; each is a CTest label of SushiProfilerTests.cmake."""

    SMALL = "small"
    MEDIUM = "medium"
    LARGE = "large"


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

_RUNNER = Runner(console, PROFILE.program)
_DRIVER = CMakeDriver(console, _RUNNER)


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


def size_label_regex(sizes: Sequence[Size]) -> str | None:
    """Returns the ctest -L regex that selects *sizes*, or None for every size."""
    if not sizes:
        return None
    return "^(" + "|".join(sorted({size.value for size in sizes})) + ")$"


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


def _configure(cfg: Config, tree: Path, cmake_type: str, command: list[str],
               defines: Sequence[str], root: Path, env: dict[str, str]) -> int:
    """Configures *tree* with *command* and the gate, once the tree is known to be its own.

    Raises:
        sushicore.errors.ConfigError: *tree* is configured as another build type than *cmake_type*.
        sushicore.build_admission.BuildStoppedError: the owner stopped builds of *tree*.
    """
    refuse_another_build_type(tree, cmake_type, defines)
    build_admission.refuse_a_stopped_gate(tree)
    return _DRIVER.configure([*command, *_gate_arguments(cfg, tree)], cwd=root, env=env)


def _compile(cfg: Config, tree: Path, root: Path, env: dict[str, str], config: str) -> int:
    """Compiles *tree* at the hardware thread count; the gate holds each edge back until it fits."""
    return _DRIVER.compile(cfg, tree, cwd=root, env=env, config=config,
                           jobs=build_admission.hardware_threads())


def _needs_configure(cfg: Config, tree: Path, build_type: BuildType,
                     sanitizer: Sanitizer | None, switches: dict[str, str]) -> bool:
    """Returns whether *tree* must be configured before it is compiled."""
    if not (tree / "CMakeCache.txt").is_file():
        return True
    expect = {"SP_SANITIZE": sanitizer.value if sanitizer else "", **switches}
    if not cfg.is_multi_config():
        expect["CMAKE_BUILD_TYPE"] = K_CMAKE_BUILD_TYPE[build_type]
    generator = cfg.resolved_generator()
    if generator:
        return _DRIVER.needs_configure(tree, generator, expect=expect)
    return any(cmake_cache.cached_value(tree, key) != value for key, value in expect.items())


def build(build_type: BuildType | None = None, clean: bool = False,
          defines: Sequence[str] = (), sanitizer: Sanitizer | None = None,
          benchmarks: bool | None = None, lean: bool = False,
          tree_path: str | None = None, report: bool = False) -> int:
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

    Returns:
        The exit code; 2 when *lean* and ``benchmarks=True`` ask for opposite things.
    """
    if lean and benchmarks:
        console.error("--lean builds no benchmark programs, so --benchmarks cannot join it")
        return 2
    defines = [*defines, *benchmarks_defines(benchmarks)]
    switches = tree_switches(lean)
    root = find_project_root()
    cfg = load_config()
    build_type = resolve_build_type(build_type)
    tree = build_dir(root, sanitizer, build_type, tree_path)
    if clean:
        _DRIVER.clean_tree(tree)
    env = load_build_env(cfg, tree)

    if defines or _needs_configure(cfg, tree, build_type, sanitizer, switches):
        command = [_DRIVER.cmake(cfg), "-S", str(root), "-B", str(tree),
                   f"-DCMAKE_BUILD_TYPE={K_CMAKE_BUILD_TYPE[build_type]}",
                   f"-DSP_SANITIZE={sanitizer.value if sanitizer else ''}",
                   *(f"-D{name}={value}" for name, value in switches.items()),
                   *_compiler_arguments(cfg)]
        for define in defines:
            command += ["-D", define]
        rc = _configure(cfg, tree, K_CMAKE_BUILD_TYPE[build_type], command, defines, root, env)
        if rc != 0:
            return rc

    offset = build_report.ninja_log_offset(tree)
    rc = _compile(cfg, tree, root, env, K_CMAKE_BUILD_TYPE[build_type])
    if report:
        _print_report(tree, offset)
    return rc


def _print_report(tree: Path, offset: int) -> None:
    """Prints the report of the build that appended to the log of *tree* from *offset*, and
    writes it into *tree*."""
    measured = build_report.of_build(tree, offset, build_admission.state_dir(tree))
    for line in measured.lines():
        console.info(line)
    console.info(f"Report: {measured.write(tree)}")


def test(build_type: BuildType | None = None, sizes: Sequence[Size] = (), clean: bool = False,
         filter: str | None = None, sanitizer: Sanitizer | None = None,
         jobs: int | None = None, package: bool = False) -> int:
    """Builds when needed, then runs ctest over the tests *sizes* and *filter* select.

    Args:
        sizes: The sizes to run; none runs every size.
        filter: A ctest -R regex over the `Suite.Case` names.
        sanitizer: Builds and tests that sanitizer's tree.
        jobs: Tests run at once; none leaves the count to ctest.
        package: Also installs the tree, builds tests/package/ against the install and runs
            its test.
    """
    build_type = resolve_build_type(build_type)
    rc = build(build_type, clean=clean, sanitizer=sanitizer)
    if rc != 0:
        return rc

    root = find_project_root()
    cfg = load_config()
    tree = build_dir(root, sanitizer, build_type)
    config = K_CMAKE_BUILD_TYPE[build_type]
    env = load_build_env(cfg, tree)
    rc = _DRIVER.ctest_run(cfg, tree, env, label_regex=size_label_regex(sizes),
                           exclude_label_regex="quarantined", filter=filter, jobs=jobs,
                           config=config)
    if rc != 0 or not package:
        return rc
    return _test_package(root, cfg, tree, env, config)


def _test_package(root: Path, cfg: Config, tree: Path, env: dict[str, str], config: str) -> int:
    """Installs *tree*, then builds and runs the consumer of tests/package/ against the install."""
    prefix = tree / "package-prefix"
    consumer = tree / "package"
    console.header("Package consumer")
    rc = _RUNNER.run([_DRIVER.cmake(cfg), "--install", str(tree), "--config", config,
                      "--prefix", str(prefix)], root, env)
    if rc != 0:
        return rc
    rc = _configure(cfg, consumer, config,
                    [_DRIVER.cmake(cfg), "-S", str(root / K_PACKAGE_SOURCE),
                     "-B", str(consumer), f"-DCMAKE_BUILD_TYPE={config}",
                     f"-DCMAKE_PREFIX_PATH={prefix}", *_compiler_arguments(cfg)],
                    (), root, env)
    if rc != 0:
        return rc
    rc = _compile(cfg, consumer, root, env, config)
    if rc != 0:
        return rc
    return _DRIVER.ctest_run(cfg, consumer, env, config=config)


def bench(build_type: BuildType | None = None) -> int:
    """Runs every built benchmark program of the plain tree in name order.

    Args:
        build_type: The build whose programs run; release when None, the type that ships.

    Returns:
        The first non-zero exit code among the programs, or 1 when the tree has none.
    """
    root = find_project_root()
    cfg = load_config()
    build_type = resolve_build_type(build_type, K_BENCHMARK_TYPE)
    tree = build_dir(root, None, build_type)
    config = K_CMAKE_BUILD_TYPE[build_type]
    programs = benchmark_programs(tree / K_BENCHMARK_FOLDER / config)
    if not programs:
        console.error(f"no benchmark program in {tree / K_BENCHMARK_FOLDER / config}; "
                      f"run {PROFILE.program} build --benchmarks --type release first")
        return 1
    env = load_build_env(cfg, tree)
    code = 0
    for program in programs:
        console.header(program.name)
        code = code or _RUNNER.run([str(program)], root, env)
    return code


def record_program(folder: Path) -> Path | None:
    """Returns the sp_record program in *folder*, or None when the build has not made it."""
    for name in (f"{K_RECORD_PROGRAM}.exe", K_RECORD_PROGRAM):
        if (folder / name).is_file():
            return folder / name
    return None


def record(arguments: Sequence[str], build_type: BuildType | None = None) -> int:
    """Runs sp_record of the plain tree on *arguments*, from the current folder.

    Args:
        arguments: The command, then its arguments, as sp_record takes them.
        build_type: The build whose program runs; debug when None.

    Returns:
        sp_record's exit code, or 1 when the tree holds no sp_record.
    """
    root = find_project_root()
    cfg = load_config()
    build_type = resolve_build_type(build_type)
    tree = build_dir(root, None, build_type)
    folder = tree / K_APPLICATION_FOLDER / K_CMAKE_BUILD_TYPE[build_type]
    program = record_program(folder)
    if program is None:
        console.error(f"no {K_RECORD_PROGRAM} in {folder}; run {PROFILE.program} build first")
        return 1
    return _RUNNER.run([str(program), *arguments], Path.cwd(), load_build_env(cfg, tree))


def check(report: bool = False, rule: str | None = None) -> int:
    """Runs the shared checkers under tools/.

    Args:
        report: Makes each checker print counts per rule and exit zero.
        rule: The one rule to run; only the checkers whose --list-rules names it run.

    Returns:
        The highest exit code among the checkers that ran, or 1 when no checker has *rule*.
    """
    root = find_project_root()
    checkers = [checker for checker in K_CHECKERS
                if rule is None or rule in rules_of(root / checker)]
    if not checkers:
        console.error(f"no checker under tools/ has the rule {rule}")
        return 1
    arguments = (["--report"] if report else []) + (["--rule", rule] if rule else [])
    return max(_RUNNER.run([sys.executable, str(root / checker), str(root), *arguments], root)
               for checker in checkers)


def rules_of(checker: Path) -> frozenset[str]:
    """Returns the rule names the checker at *checker* prints for --list-rules."""
    listed = subprocess.run([sys.executable, str(checker), "--list-rules"],
                            capture_output=True, text=True, check=False)
    return frozenset(listed.stdout.split()) if listed.returncode == 0 else frozenset()
