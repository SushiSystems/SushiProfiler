# conftest.py
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""Fixtures of the `sp` tests, and the size every test must carry.

Each test carries exactly one size mark, which gives it its pytest-timeout.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

import pytest
from sushicore.console_memory import RecordingConsole
from sushicore.proc_memory import RecordingRunner

from sushiprofiler.config import Config
from sushiprofiler.services.project import Workspace

K_SIZE_TIMEOUTS = {"small": 60, "medium": 300, "large": 900}


def _sizes_of(item: pytest.Item) -> list[str]:
    """Returns the size marks *item* carries."""
    return [mark.name for mark in item.iter_markers() if mark.name in K_SIZE_TIMEOUTS]


def pytest_collection_modifyitems(items: list[pytest.Item]) -> None:
    """Fails collection when a test carries no size or two, and gives each its timeout."""
    wrong = [f"{item.nodeid}: {_sizes_of(item) or 'no size'}"
             for item in items if len(_sizes_of(item)) != 1]
    if wrong:
        raise pytest.UsageError(f"every test carries exactly one size: {', '.join(wrong)}")
    for item in items:
        item.add_marker(pytest.mark.timeout(K_SIZE_TIMEOUTS[_sizes_of(item)[0]]))


@dataclass
class Project:
    """Holds a scratch checkout and what the project service did to it."""

    root: Path
    runner: RecordingRunner
    console: RecordingConsole
    workspace: Workspace

    @property
    def commands(self) -> list[list[str]]:
        """Returns the argv of every command the runner was handed, in order."""
        return [list(command.argv) for command in self.runner.commands]

    def tree_of(self, build_type: str = "Debug", name: str = "default") -> Path:
        """Returns the tree `build/<name>-<type>` of *build_type*, the way sushicore names it."""
        return self.root / "build" / f"{name}-{build_type.lower()}"

    def configure_tree(self, build_type: str = "Debug", sanitize: str = "",
                       name: str = "default", tests: str = "ON") -> Path:
        """Plants the files a finished Ninja configure of *build_type* leaves behind.

        Returns:
            The tree, `build/<name>-<type>`.
        """
        tree = self.tree_of(build_type, name)
        tree.mkdir(parents=True, exist_ok=True)
        (tree / "build.ninja").write_text("", encoding="utf-8")
        (tree / "CMakeCache.txt").write_text(
            f"CMAKE_BUILD_TYPE:STRING={build_type}\nCMAKE_GENERATOR:INTERNAL=Ninja\n"
            f"SP_SANITIZE:STRING={sanitize}\nSP_BUILD_TESTS:BOOL={tests}\n"
            f"SP_BUILD_APPLICATIONS:BOOL={tests}\n",
            encoding="utf-8")
        return tree


@pytest.fixture
def project(tmp_path) -> Project:
    """Returns a scratch checkout whose workspace records argv and starts nothing."""
    (tmp_path / "CMakeLists.txt").write_text("project(x)\n", encoding="utf-8")
    console = RecordingConsole()
    runner = RecordingRunner(console)
    cfg = Config(cxx="clang++", generator="Ninja", ninja_exe="ninja")
    workspace = Workspace(tmp_path, cfg, console, runner, environment={})
    return Project(tmp_path, runner, console, workspace)
