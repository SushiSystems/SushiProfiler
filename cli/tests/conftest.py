# conftest.py
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""Fixtures of the `sp` tests, and the size every test must carry.

Each test carries exactly one size mark, which gives it its pytest-timeout.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path

import pytest

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


class RecordingRunner:
    """Records every command line it is handed and starts none of them."""

    def __init__(self) -> None:
        """Starts with no command recorded."""
        self.commands: list[list[str]] = []

    def resolve_exe(self, name: str, env: dict[str, str] | None = None) -> str:
        """Returns the name unchanged."""
        return name

    def run(self, cmd: list[str], cwd: Path, env: dict[str, str] | None = None) -> int:
        """Records the command and reports success."""
        self.commands.append([str(part) for part in cmd])
        return 0

    def run_drained(self, cmd: list[str], cwd: Path, env: dict[str, str] | None = None) -> int:
        """Records the command and reports success."""
        return self.run(cmd, cwd, env)


class RecordingConsole:
    """Records every line a service prints, keyed by the helper that printed it."""

    def __init__(self) -> None:
        """Starts with no line recorded."""
        self.lines: list[tuple[str, str]] = []

    def __getattr__(self, kind: str):
        """Returns a printer that records its text under the helper's name."""
        return lambda text="": self.lines.append((kind, str(text)))

    def of(self, kind: str) -> list[str]:
        """Returns the texts printed through one helper, in order."""
        return [text for printed, text in self.lines if printed == kind]


@dataclass
class Project:
    """Holds a scratch checkout and what the project service did to it."""

    root: Path
    runner: RecordingRunner
    console: RecordingConsole

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
def project(tmp_path, monkeypatch) -> Project:
    """Returns a scratch checkout whose project service records argv and starts nothing."""
    from sushicore.cmake_driver import CMakeDriver

    from sushiprofiler.config import Config
    from sushiprofiler.services import project as project_service

    (tmp_path / "CMakeLists.txt").write_text("project(x)\n", encoding="utf-8")
    runner, console = RecordingRunner(), RecordingConsole()
    cfg = Config(cxx="clang++", generator="Ninja", ninja_exe="ninja")
    monkeypatch.setattr(project_service, "find_project_root", lambda: tmp_path)
    monkeypatch.setattr(project_service, "load_config", lambda: cfg)
    monkeypatch.setattr(project_service, "load_build_env", lambda cfg, tree: {})
    monkeypatch.setattr(project_service, "console", console)
    monkeypatch.setattr(project_service, "_RUNNER", runner)
    monkeypatch.setattr(project_service, "_DRIVER", CMakeDriver(console, runner))
    return Project(tmp_path, runner, console)
