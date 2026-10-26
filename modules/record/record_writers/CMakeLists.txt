# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

sushiprofiler_add_module(
    NAME record_writers TIER record
    SOURCES
        record_file_writer.cpp
        terminal_writer.cpp
    DEPENDS analysis record
    PRIVATE_DEPENDS record_json)
