# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.

sushiprofiler_add_module(
    NAME checks TIER record
    SOURCES
        check.cpp
        verdict.cpp
    DEPENDS version record
    PRIVATE_DEPENDS analysis json statistics)
