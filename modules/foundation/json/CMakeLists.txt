# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

sushiprofiler_add_module(
    NAME json TIER foundation
    SOURCES
        json_field_reader.cpp
        json_reader.cpp
        json_value.cpp
        json_writer.cpp
    DEPENDS version)
