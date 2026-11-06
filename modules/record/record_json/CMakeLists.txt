# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.

sushiprofiler_add_module(
    NAME record_json TIER record
    SOURCES
        record_json_reader.cpp
        record_json_writer.cpp
        record_names.cpp
    DEPENDS version record
    PRIVATE_DEPENDS json sites)
