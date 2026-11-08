# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.

sushiprofiler_add_module_tests(
    MODULE stream_writers
    SOURCES
        test_chrome_trace_writer.cpp
        test_memory_stream_writer.cpp
        test_stream_writer.cpp)
