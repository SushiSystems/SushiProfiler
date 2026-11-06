# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.

sushiprofiler_add_module(
    NAME record TIER record
    SOURCES
        device_span_keys.cpp
        period_record.cpp
        run_record.cpp
        sample_series.cpp
        span_table.cpp
    DEPENDS version events sites)
