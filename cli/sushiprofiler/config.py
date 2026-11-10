# config.py
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""Loads the layered configuration of `sp` and finds the compiler and generator it builds with.

The layers and the compiler search order are in cli/README.md, "config.py".
"""

from __future__ import annotations

from dataclasses import dataclass

from sushicore.config_base import ToolConfig
from sushicore.module_config import ModuleConfig
from sushicore.profile import ModuleProfile

from .host import Host
from .host_system import SystemHost

PROFILE = ModuleProfile(
    name="SushiProfiler",
    program="sp",
    env_prefix="SP",
    root_marker="CMakeLists.txt",
    env_fields=("cxx", "generator", "cmake_exe", "ctest_exe", "ninja_exe", "vs_vcvars"),
)

#: The compiler a consumer's package build hands the CLI; it outranks the configured one.
K_STACK_CXX_ENV = "SUSHISTACK_CXX"

_MODULE = ModuleConfig(PROFILE, use_workspace_config=True)

find_project_root = _MODULE.find_project_root
config_dir = _MODULE.config_dir


@dataclass
class Config(ToolConfig):
    """Holds the resolved tool configuration of the SushiProfiler build for this platform."""

    # Empty, so CMake picks the platform default when no compiler is found.
    generator: str = ""

    # The compiler SUSHISTACK_CXX names; empty outside a package build. It outranks `cxx`.
    stack_cxx: str = ""

    def resolved_compiler(self, host: Host | None = None) -> str:
        """Returns the C++ compiler: SUSHISTACK_CXX, else the pinned one, else clang++ on *host*."""
        if self.stack_cxx:
            return self.expand(self.stack_cxx)
        if self.cxx:
            return self.expand(self.cxx)
        return (host or SystemHost()).which("clang++") or ""

    def resolved_ninja(self, host: Host | None = None) -> str:
        """Returns the ninja to hand CMake, or '' when none is pinned and none is on *host*."""
        if self.ninja_exe:
            return self.expand(self.ninja_exe)
        return (host or SystemHost()).which("ninja") or ""

    def resolved_generator(self, host: Host | None = None) -> str:
        """Returns the pinned generator, else Ninja with a compiler and ninja found, else ''."""
        if self.generator:
            return self.generator
        if self.resolved_compiler(host) and self.resolved_ninja(host):
            return "Ninja"
        return ""

    def is_multi_config(self, host: Host | None = None) -> bool:
        """Returns whether the generator picks the build type at build time (Visual Studio)."""
        generator = self.resolved_generator(host)
        if generator == "":
            return self.is_windows
        return generator.startswith(("Visual Studio", "Xcode"))


def load_config(host: Host | None = None) -> Config:
    """Returns the layered configuration, with SUSHISTACK_CXX read from *host*."""
    cfg = _MODULE.load(Config)
    cfg.stack_cxx = (host or SystemHost()).variable(K_STACK_CXX_ENV)
    return cfg
