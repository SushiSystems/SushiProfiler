# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.

# CMakeLists.txt

sushiprofiler_add_module_tests(
    MODULE umbrella
    SOURCES
        test_SushiProfiler.cpp)
