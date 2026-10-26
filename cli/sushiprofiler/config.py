# config.py
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""Loads the layered configuration of `sp` and finds the compiler and generator it builds with.

The layers and the compiler search order are in cli/README.md, "config.py".
"""

from __future__ import annotations

import shutil
from dataclasses import dataclass

from sushicore.config_base import ToolConfig
from sushicore.module_config import ModuleConfig
from sushicore.profile import ModuleProfile

PROFILE = ModuleProfile(
    name="SushiProfiler",
    program="sp",
    env_prefix="SP",
    root_marker="CMakeLists.txt",
    env_fields=("cxx", "generator", "cmake_exe", "ctest_exe", "ninja_exe", "vs_vcvars"),
)

_MODULE = ModuleConfig(PROFILE, use_workspace_config=True)

find_project_root = _MODULE.find_project_root
config_dir = _MODULE.config_dir


@dataclass
class Config(ToolConfig):
    """Holds the resolved tool configuration of the SushiProfiler build for this platform."""

    # Empty, so CMake picks the platform default when no compiler is found.
    generator: str = ""

    def resolved_compiler(self) -> str:
        """Returns the C++ compiler to build with: the pinned one, else clang++ on PATH, else ''."""
        if self.cxx:
            return self.expand(self.cxx)
        return shutil.which("clang++") or ""

    def resolved_ninja(self) -> str:
        """Returns the ninja to hand CMake, or '' when none is found."""
        if self.ninja_exe:
            return self.expand(self.ninja_exe)
        return shutil.which("ninja") or ""

    def resolved_generator(self) -> str:
        """Returns the pinned generator, else Ninja with a found compiler and ninja, else ''."""
        if self.generator:
            return self.generator
        if self.resolved_compiler() and self.resolved_ninja():
            return "Ninja"
        return ""

    def is_multi_config(self) -> bool:
        """Returns whether the generator picks the build type at build time (Visual Studio)."""
        generator = self.resolved_generator()
        if generator == "":
            return self.is_windows
        return generator.startswith(("Visual Studio", "Xcode"))


def load_config() -> Config:
    """Returns the layered configuration for the current platform."""
    return _MODULE.load(Config)
