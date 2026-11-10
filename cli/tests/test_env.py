# test_env.py
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""Tests the environment `sp` hands cmake and ctest, on an in-memory host."""

from __future__ import annotations

import os
from pathlib import Path

import pytest

from sushiprofiler import env
from sushiprofiler.config import Config
from sushiprofiler.host_memory import MemoryHost


@pytest.mark.small
class TestWithoutACompiler:
    """Covers the path that returns before the build folder is touched."""

    def test_the_environment_is_the_hosts(self):
        host = MemoryHost(variables={"A": "1", "B": "2"})

        built = env.load_build_env(Config(), Path("unused"), host)

        assert built == {"A": "1", "B": "2"}

    def test_the_environment_is_a_copy(self):
        host = MemoryHost(variables={"A": "1"})

        built = env.load_build_env(Config(), Path("unused"), host)
        built["B"] = "2"

        assert host.environment() == {"A": "1"}


@pytest.mark.medium
class TestWithACompiler:
    """Covers the path that creates the build folder under a temporary folder."""

    def test_a_found_compiler_comes_first_on_path(self, tmp_path):
        compiler = tmp_path / "bin" / "clang++"
        host = MemoryHost(variables={"PATH": "/usr/bin"})
        cfg = Config(cxx=str(compiler), platform="linux")

        built = env.load_build_env(cfg, tmp_path / "build", host)

        assert built["PATH"].split(os.pathsep)[0] == str(compiler.parent)
