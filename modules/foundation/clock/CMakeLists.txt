# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.

sushiprofiler_add_module(
    NAME clock TIER foundation
    SOURCES
        manual_tick_source.cpp
        trace_clock.cpp
        trace_tick_source.cpp
    DEPENDS version)
