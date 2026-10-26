# test_config.py
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""Tests how `sp` picks its compiler and generator."""

from __future__ import annotations

import shutil

import pytest

from sushiprofiler.config import Config

pytestmark = pytest.mark.small


def test_a_pinned_compiler_wins(monkeypatch):
    monkeypatch.setattr(shutil, "which", lambda name: f"/usr/bin/{name}")

    assert Config(cxx="/opt/clang++").resolved_compiler() == "/opt/clang++"


def test_without_a_pin_the_clang_on_path_is_used(monkeypatch):
    monkeypatch.setattr(shutil, "which",
                        lambda name: "/usr/bin/clang++" if name == "clang++" else None)

    assert Config().resolved_compiler() == "/usr/bin/clang++"


def test_without_any_compiler_cmake_chooses(monkeypatch):
    monkeypatch.setattr(shutil, "which", lambda name: None)

    cfg = Config()
    assert cfg.resolved_compiler() == ""
    assert cfg.resolved_generator() == ""


def test_a_found_compiler_and_ninja_choose_ninja(monkeypatch):
    monkeypatch.setattr(shutil, "which", lambda name: f"/usr/bin/{name}")

    cfg = Config()
    assert cfg.resolved_generator() == "Ninja"
    assert not cfg.is_multi_config()


def test_a_visual_studio_generator_is_multi_config():
    assert Config(generator="Visual Studio 17 2022").is_multi_config()
