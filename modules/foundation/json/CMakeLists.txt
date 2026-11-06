# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.

sushiprofiler_add_module(
    NAME json TIER foundation
    SOURCES
        json_field_reader.cpp
        json_reader.cpp
        json_value.cpp
        json_writer.cpp
    DEPENDS version)
