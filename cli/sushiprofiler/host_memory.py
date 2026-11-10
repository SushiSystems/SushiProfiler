# host_memory.py
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""A host that answers from the maps it was built with, for tests of code that reads the host."""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import final


@final
@dataclass(frozen=True, slots=True)
class MemoryHost:
    """Answers the `Host` port from two maps and touches neither the process nor the disk.

    Attributes:
        variables: The environment variables, name to value.
        executables: The executables that are found, name to path.
    """

    variables: dict[str, str] = field(default_factory=dict)
    executables: dict[str, str] = field(default_factory=dict)

    def environment(self) -> dict[str, str]:
        """Returns a copy of the variables."""
        return dict(self.variables)

    def variable(self, name: str) -> str:
        """Returns the value of the variable *name*, or '' when it is not in the variables."""
        return self.variables.get(name, "")

    def which(self, name: str) -> str | None:
        """Returns the path the executables map gives *name*, or None."""
        return self.executables.get(name)
