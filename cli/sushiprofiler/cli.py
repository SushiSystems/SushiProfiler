# cli.py
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""Declares the commands of `sp`, the SushiProfiler developer CLI.

A thin Typer layer: each command parses its arguments and delegates to
``sushiprofiler.services.project``. The root options, the `docs` group and the entry point come from
sushicore.
"""

from __future__ import annotations

from typing import Any, Optional

import typer
from sushicore.docs_bundle import register_docs_commands
from sushicore.entry import run as run_app
from sushicore.root_options import register_root_options
from sushicore.typer_help import help_group

from . import console
from .config import find_project_root
from .services import project as project_service
from .services.project import BuildType, Sanitizer, Size

K_PROGRAM = "sp"
K_DISTRIBUTION = "sushiprofiler-cli"
K_PANEL = "Build"

K_TYPE_HELP = ("Build type: debug | release | relwithdebinfo. "
               "Defaults to the type the build tree carries, else debug.")

_help_group = help_group(console.current)

app = typer.Typer(
    name=K_PROGRAM,
    cls=_help_group,
    help="SushiProfiler developer CLI: build, test, bench and check SushiProfiler.",
    no_args_is_help=True,
    add_completion=False,
    rich_markup_mode="rich",
)


def _type_option() -> Any:
    """Returns the default of a `--type/-t` parameter."""
    return typer.Option(None, "--type", "-t", case_sensitive=False, help=K_TYPE_HELP)


def _sanitize_option() -> Any:
    """Returns the default of a `--sanitize` parameter."""
    return typer.Option(None, "--sanitize", case_sensitive=False,
                        help="Sanitizer build in its own tree: address (ASan and UBSan) | "
                             "thread (TSan).")


def _clean_option() -> Any:
    """Returns the default of a `--clean` parameter."""
    return typer.Option(False, "--clean", help="Delete the build tree before configuring.")


@app.command(
    "build",
    rich_help_panel=K_PANEL,
    epilog="sp build\n"
           "sp build --type release\n"
           "sp build --benchmarks --type release  # Also build the benchmark programs\n"
           "sp build --sanitize address  # ASan and UBSan, in build-asan/",
)
def build(
    type: Optional[BuildType] = _type_option(),
    clean: bool = _clean_option(),
    define: Optional[list[str]] = typer.Option(
        None, "--define", "-D", metavar="VAR=VALUE",
        help="Pass a CMake cache variable; a configured tree reconfigures. Repeatable."),
    sanitize: Optional[Sanitizer] = _sanitize_option(),
    benchmarks: Optional[bool] = typer.Option(
        None, "--benchmarks/--no-benchmarks",
        help="Build the benchmark programs beside the tests, or stop building them. "
             "When omitted the tree keeps its setting."),
) -> None:
    """Configure when needed, then build the library and the tests."""
    raise typer.Exit(project_service.build(type, clean=clean, defines=define or (),
                                           sanitizer=sanitize, benchmarks=benchmarks))


@app.command(
    "test",
    rich_help_panel=K_PANEL,
    epilog="sp test  # Every size\n"
           "sp test --label small --label medium  # What a pull request runs\n"
           "sp test --filter 'Small_Version.*' --jobs 4\n"
           "sp test --package  # Also build tests/package/ against the install",
)
def test(
    type: Optional[BuildType] = _type_option(),
    label: Optional[list[Size]] = typer.Option(
        None, "--label", "-l", case_sensitive=False,
        help="Test size to run: small | medium | large; repeat for more. Default: every size."),
    filter: Optional[str] = typer.Option(
        None, "--filter", "-f", help="ctest -R regex over 'Suite.Case' test names."),
    jobs: Optional[int] = typer.Option(
        None, "--jobs", "-j", min=1,
        help="Tests run at once. When omitted ctest decides."),
    clean: bool = _clean_option(),
    sanitize: Optional[Sanitizer] = _sanitize_option(),
    package: bool = typer.Option(
        False, "--package",
        help="Install the tree, build tests/package/ against it and run its test."),
) -> None:
    """Build when needed, then run ctest over the sizes and the filter."""
    raise typer.Exit(project_service.test(type, label or (), clean=clean, filter=filter,
                                          sanitizer=sanitize, jobs=jobs, package=package))


@app.command(
    "bench",
    rich_help_panel=K_PANEL,
    epilog="sp build --benchmarks --type release\n"
           "sp bench --type release  # Every benchmark program, in name order",
)
def bench(type: Optional[BuildType] = _type_option()) -> None:
    """Run every built benchmark program.

    A benchmark measures and does not pass or fail, so it is not a test. Build the programs
    with sp build --benchmarks first.
    """
    raise typer.Exit(project_service.bench(type))


@app.command(
    "check",
    rich_help_panel=K_PANEL,
    epilog="sp check\n"
           "sp check --report  # Counts per rule, without failing\n"
           "sp check --rule rule_console_output  # One rule",
)
def check(
    report: bool = typer.Option(False, "--report", "-r", help="Report counts without failing."),
    rule: Optional[str] = typer.Option(None, "--rule", help="Run only this rule."),
) -> None:
    """Run the shared checkers under tools/."""
    raise typer.Exit(project_service.check(report=report, rule=rule))


register_docs_commands(
    app,
    program=K_PROGRAM,
    panel=K_PANEL,
    project_root=find_project_root,
    report=lambda line: console.success(line),
    group_cls=_help_group,
)

register_root_options(app, distribution=K_DISTRIBUTION)


def _report_error(message: str) -> None:
    """Prints one error line through the console, which is built only at this call."""
    console.error(message)


def main() -> None:
    """Runs `sp` and exits; a failure sushicore reports becomes one line and exit code 1."""
    run_app(app, _report_error)


if __name__ == "__main__":
    main()
