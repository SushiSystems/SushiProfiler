# test_config.py
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""Tests how `sp` picks its compiler and generator, on an in-memory host."""

from __future__ import annotations

import pytest

from sushiprofiler.config import Config
from sushiprofiler.host_memory import MemoryHost

pytestmark = pytest.mark.small

K_BOTH_FOUND = MemoryHost(executables={"clang++": "/usr/bin/clang++", "ninja": "/usr/bin/ninja"})
K_NONE_FOUND = MemoryHost()


def test_a_pinned_compiler_wins():
    assert Config(cxx="/opt/clang++").resolved_compiler(K_BOTH_FOUND) == "/opt/clang++"


@pytest.mark.parametrize(
    ("stack_cxx", "expected"), [("/stack/clang++", "/stack/clang++"), ("", "/pinned/clang++")])
def test_the_compiler_the_environment_names_outranks_the_pinned_one(stack_cxx, expected):
    cfg = Config(cxx="/pinned/clang++", stack_cxx=stack_cxx)

    assert cfg.resolved_compiler(K_BOTH_FOUND) == expected


def test_without_a_pin_the_clang_on_path_is_used():
    assert Config().resolved_compiler(K_BOTH_FOUND) == "/usr/bin/clang++"


def test_without_any_compiler_cmake_chooses():
    cfg = Config()

    assert cfg.resolved_compiler(K_NONE_FOUND) == ""
    assert cfg.resolved_generator(K_NONE_FOUND) == ""


def test_a_found_compiler_and_ninja_choose_ninja():
    cfg = Config()

    assert cfg.resolved_generator(K_BOTH_FOUND) == "Ninja"
    assert not cfg.is_multi_config(K_BOTH_FOUND)


def test_a_visual_studio_generator_is_multi_config():
    assert Config(generator="Visual Studio 17 2022").is_multi_config(K_NONE_FOUND)
