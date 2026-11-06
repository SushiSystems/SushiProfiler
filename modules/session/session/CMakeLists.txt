# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.

sushiprofiler_add_module(
    NAME session TIER session
    SOURCES
        session.cpp
    DEPENDS version analysis config consumer live periods record record_writers registry sources
            stream_writers window
    PRIVATE_DEPENDS collector environment sites)
