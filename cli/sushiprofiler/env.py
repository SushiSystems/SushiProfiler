# env.py
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""Assembles the environment cmake and ctest run under for the compiler `sp` found.

What a found clang++ needs on Windows and how the snapshot is cached are in cli/README.md, "env.py".
"""

from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path

from sushicore.build_env import merge_env, prepend_path, read_cache, snapshot_windows, write_cache

from . import console
from .config import Config

K_CACHE_NAME = ".sushiprofiler_env.json"


def _cache_key(cfg: Config) -> str:
    """Returns the key that changes whenever the snapshot would."""
    material = json.dumps(
        {"platform": cfg.platform, "vs_vcvars": cfg.vs_vcvars, "compiler": cfg.resolved_compiler()},
        sort_keys=True,
    )
    return hashlib.sha256(material.encode()).hexdigest()


def load_build_env(cfg: Config, build_dir: Path) -> dict[str, str]:
    """Returns the environment of the cmake and ctest subprocesses.

    Args:
        cfg: The resolved configuration.
        build_dir: Where the Visual Studio snapshot is cached.

    Returns:
        A copy of ``os.environ`` when no compiler was found; else it with the Visual Studio
        snapshot on Windows and, for a compiler given by its path, its folder first on PATH.
    """
    compiler = cfg.resolved_compiler()
    if not compiler:
        return dict(os.environ)

    build_dir.mkdir(parents=True, exist_ok=True)
    cache_file = build_dir / K_CACHE_NAME
    key = _cache_key(cfg)
    snapshot = read_cache(cache_file, key)
    if snapshot is None and cfg.is_windows:
        snapshot = snapshot_windows(cfg, console)
        if snapshot:
            write_cache(cache_file, key, snapshot)

    env = merge_env(os.environ, snapshot) if snapshot else dict(os.environ)
    # A bare name such as `cl` is already found on PATH; only a path names a folder to add.
    if Path(compiler).is_absolute():
        prepend_path(env, "PATH", [str(Path(compiler).parent)])
    return env
