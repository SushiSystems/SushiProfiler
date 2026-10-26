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
    help="SushiProfiler developer CLI: build, test, bench and check SushiProfiler, and read "
         "its run records.",
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


K_RECORD_PANEL = "Records"

record_app = typer.Typer(
    name="record",
    cls=_help_group,
    help="Show, compare, check and validate run records with the sp_record program.",
    no_args_is_help=True,
    add_completion=False,
    rich_markup_mode="rich",
)
app.add_typer(record_app, name="record", rich_help_panel=K_RECORD_PANEL)


def _repeated(option: str, values: Optional[list[str]]) -> list[str]:
    """Returns *option* before each of *values*, as sp_record takes a repeated option."""
    words: list[str] = []
    for value in values or ():
        words += [option, value]
    return words


@record_app.command(
    "show",
    epilog="sp record show build/profiles/run.sprec.json\n"
           "sp record show run.sprec.json --analysis breakdown --analysis top_spans --top 10\n"
           "sp record show run.sprec.json --analysis overhead --baseline off.sprec.json",
)
def record_show(
    record: str = typer.Argument(..., help="The record file."),
    analysis: Optional[list[str]] = typer.Option(
        None, "--analysis", "-a", help="An analysis to run; repeat for more. Default: six."),
    top: Optional[int] = typer.Option(None, "--top", min=1, help="Rows a ranking keeps."),
    group_by: Optional[list[str]] = typer.Option(
        None, "--group-by", help="An attribute key top_spans groups by; repeat for more."),
    baseline: Optional[str] = typer.Option(
        None, "--baseline", help="A record of the run without profiling, for overhead."),
    type: Optional[BuildType] = _type_option(),
) -> None:
    """Print a record's analyses as tables."""
    arguments = ["show", record, *_repeated("--analysis", analysis),
                 *_repeated("--group-by", group_by)]
    if top is not None:
        arguments += ["--top", str(top)]
    if baseline is not None:
        arguments += ["--baseline", baseline]
    raise typer.Exit(project_service.record(arguments, type))


@record_app.command(
    "compare",
    epilog="sp record compare before.sprec.json after.sprec.json\n"
           "sp record compare release.sprec.json debug.sprec.json --allow-build-mismatch",
)
def record_compare(
    baseline: str = typer.Argument(..., help="The record compared against."),
    candidate: str = typer.Argument(..., help="The record compared."),
    allow_build_mismatch: bool = typer.Option(
        False, "--allow-build-mismatch", help="Compare records of different builds."),
    type: Optional[BuildType] = _type_option(),
) -> None:
    """Compare two records subject by subject; exit 1 when their builds differ."""
    arguments = ["compare", baseline, candidate]
    if allow_build_mismatch:
        arguments.append("--allow-build-mismatch")
    raise typer.Exit(project_service.record(arguments, type))


@record_app.command(
    "check",
    epilog="sp record check run.sprec.json budgets.json",
)
def record_check(
    record: str = typer.Argument(..., help="The record file."),
    checks: str = typer.Argument(..., help="The check file, a JSON array of checks."),
    type: Optional[BuildType] = _type_option(),
) -> None:
    """Check a record against bounds; exit 1 when a check fails."""
    raise typer.Exit(project_service.record(["check", record, checks], type))


@record_app.command(
    "validate",
    epilog="sp record validate run.sprec.json",
)
def record_validate(
    record: str = typer.Argument(..., help="The record file."),
    type: Optional[BuildType] = _type_option(),
) -> None:
    """Read a record strictly; exit 1 when it is refused."""
    raise typer.Exit(project_service.record(["validate", record], type))


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
