# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.

sushiprofiler_add_module_tests(
    MODULE zones
    SOURCES
        allocation_counter.cpp
        test_admission.cpp
        test_macros.cpp
        test_named_macros.cpp
        test_named_records.cpp
        test_records.cpp
        test_zone.cpp
    BENCHMARKS
        benchmark_zones.cpp)
