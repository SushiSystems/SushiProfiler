# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

sushiprofiler_add_module_tests(
    MODULE statistics
    SOURCES
        test_bootstrap.cpp
        test_dispersion.cpp
        test_log2_histogram.cpp
        test_sorted_samples.cpp
        test_steady_state.cpp)
