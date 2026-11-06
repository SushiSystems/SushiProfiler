# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.

sushiprofiler_add_module(
    NAME fold TIER session
    SOURCES
        lane_tracker.cpp
        period_fold.cpp
        record_clock.cpp
    DEPENDS version clock events record)
