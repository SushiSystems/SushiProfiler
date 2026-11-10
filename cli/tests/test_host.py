# test_host.py
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""Tests the `Host` port once, against each of its two adapters (`HostContract`)."""

from __future__ import annotations

import os
import sys

import pytest

from sushiprofiler.host import Host
from sushiprofiler.host_memory import MemoryHost
from sushiprofiler.host_system import SystemHost

K_ABSENT_NAME = "SP_HOST_CONTRACT_ABSENT"


def _memory() -> tuple[Host, str, str]:
    """Returns the in-memory host, an executable it finds and the path it gives."""
    host = MemoryHost(variables={"A": "1"}, executables={"tool": "/usr/bin/tool"})
    return host, "tool", "/usr/bin/tool"


def _system() -> tuple[Host, str, str]:
    """Returns the system host, an executable it finds (this interpreter) and its path."""
    return SystemHost(), sys.executable, sys.executable


class HostContract:
    """Holds the promises of the `Host` port; a subclass supplies `make`."""

    def make(self) -> tuple[Host, str, str]:
        """Returns a host, the name of an executable it finds and the path it gives."""
        raise NotImplementedError

    def test_the_environment_is_a_copy(self):
        host, _, _ = self.make()
        before = host.environment()

        host.environment()["SP_HOST_CONTRACT_EXTRA"] = "x"

        assert host.environment() == before

    def test_a_variable_reads_what_the_environment_holds(self):
        host, _, _ = self.make()
        name = next(iter(host.environment()))

        assert host.variable(name) == host.environment()[name]

    def test_a_variable_that_is_not_set_reads_empty(self):
        host, _, _ = self.make()

        assert host.variable(K_ABSENT_NAME) == ""

    def test_an_executable_that_is_found_gives_its_path(self):
        host, name, path = self.make()

        assert host.which(name) == path

    def test_an_executable_that_is_not_found_gives_none(self):
        host, _, _ = self.make()

        assert host.which(K_ABSENT_NAME) is None


@pytest.mark.small
class TestMemoryHost(HostContract):
    """Runs the contract on the in-memory host."""

    def make(self) -> tuple[Host, str, str]:
        return _memory()


@pytest.mark.medium
class TestSystemHost(HostContract):
    """Runs the contract on the system host, which reads the real process environment."""

    def make(self) -> tuple[Host, str, str]:
        return _system()

    def test_the_environment_is_the_process_environment(self):
        assert SystemHost().environment() == dict(os.environ)
