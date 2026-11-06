# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.

sushiprofiler_add_module_tests(
    MODULE json
    SOURCES
        test_json_field_reader.cpp
        test_json_reader.cpp
        test_json_value.cpp
        test_json_writer.cpp)
