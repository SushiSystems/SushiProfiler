# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.

sushiprofiler_add_module(
    NAME stream_writers TIER session
    SOURCES
        chrome_trace_writer.cpp
    DEPENDS version record record_writers
    PRIVATE_DEPENDS json sites)
