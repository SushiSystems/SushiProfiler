# host.py
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""Declares the port through which `sp` reads the environment and locates executables.

The system adapter is in host_system.py and the in-memory one in host_memory.py.
"""

from __future__ import annotations

from typing import Protocol


class Host(Protocol):
    """Reads the environment variables and finds the executables of the machine `sp` runs on."""

    def environment(self) -> dict[str, str]:
        """Returns a copy of every environment variable; changing the copy changes nothing."""
        ...

    def variable(self, name: str) -> str:
        """Returns the value of the variable *name*, or '' when it is not set."""
        ...

    def which(self, name: str) -> str | None:
        """Returns the path of the executable *name*, or None when it is not found."""
        ...
