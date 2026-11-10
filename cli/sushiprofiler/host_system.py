# host_system.py
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""Reads the real process environment and searches the real PATH."""

from __future__ import annotations

import os
import shutil
from typing import final


@final
class SystemHost:
    """Answers the `Host` port from `os.environ` and `shutil.which`."""

    def environment(self) -> dict[str, str]:
        """Returns a copy of ``os.environ``."""
        return dict(os.environ)

    def variable(self, name: str) -> str:
        """Returns the value of the variable *name* in ``os.environ``, or '' when it is not set."""
        return os.environ.get(name, "")

    def which(self, name: str) -> str | None:
        """Returns the path ``shutil.which`` finds for *name*, or None."""
        return shutil.which(name)
