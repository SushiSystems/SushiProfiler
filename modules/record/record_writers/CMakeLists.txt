# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.

sushiprofiler_add_module(
    NAME record_writers TIER record
    SOURCES
        memory_record_writer.cpp
        record_file_writer.cpp
        terminal_writer.cpp
    DEPENDS version analysis record
    PRIVATE_DEPENDS record_json)
