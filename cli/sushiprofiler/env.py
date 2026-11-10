# env.py
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""Assembles the environment cmake and ctest run under for the compiler `sp` found.

What a found clang++ needs on Windows and how the snapshot is cached are in cli/README.md, "env.py".
"""

from __future__ import annotations

import hashlib
import json
from pathlib import Path

from sushicore.build_env import merge_env, prepend_path, read_cache, snapshot_windows, write_cache

from . import console
from .config import Config
from .host import Host
from .host_system import SystemHost

K_CACHE_NAME = ".sushiprofiler_env.json"


def _cache_key(cfg: Config, host: Host) -> str:
    """Returns the key that changes whenever the snapshot would."""
    material = json.dumps(
        {
            "platform": cfg.platform,
            "vs_vcvars": cfg.vs_vcvars,
            "compiler": cfg.resolved_compiler(host),
        },
        sort_keys=True,
    )
    return hashlib.sha256(material.encode()).hexdigest()


def load_build_env(cfg: Config, build_dir: Path, host: Host | None = None) -> dict[str, str]:
    """Returns the environment of the cmake and ctest subprocesses.

    Args:
        cfg: The resolved configuration.
        build_dir: Where the Visual Studio snapshot is cached.
        host: Where the environment and the compiler search come from; the system when None.

    Returns:
        A copy of the host environment when no compiler was found; else it with the Visual Studio
        snapshot on Windows and, for a compiler given by its path, its folder first on PATH.
    """
    host = host or SystemHost()
    compiler = cfg.resolved_compiler(host)
    if not compiler:
        return host.environment()

    build_dir.mkdir(parents=True, exist_ok=True)
    cache_file = build_dir / K_CACHE_NAME
    key = _cache_key(cfg, host)
    snapshot = read_cache(cache_file, key)
    if snapshot is None and cfg.is_windows:
        snapshot = snapshot_windows(cfg, console)
        if snapshot:
            write_cache(cache_file, key, snapshot)

    env = merge_env(host.environment(), snapshot) if snapshot else host.environment()
    # A bare name such as `cl` is already found on PATH; only a path names a folder to add.
    if Path(compiler).is_absolute():
        prepend_path(env, "PATH", [str(Path(compiler).parent)])
    return env
