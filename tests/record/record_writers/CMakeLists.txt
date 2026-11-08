# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.

sushiprofiler_add_module_tests(
    MODULE record_writers
    SOURCES
        test_memory_record_writer.cpp
        test_record_file_writer.cpp
        test_record_writer.cpp
        test_terminal_writer.cpp)
