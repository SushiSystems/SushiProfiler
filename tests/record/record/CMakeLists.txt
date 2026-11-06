# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.

sushiprofiler_add_module_tests(
    MODULE record
    SOURCES
        test_device_span_keys.cpp
        test_period_record.cpp
        test_run_record.cpp
        test_sample_series.cpp
        test_span_table.cpp)
