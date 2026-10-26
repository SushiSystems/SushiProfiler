# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

sushiprofiler_add_module(
    NAME session TIER session
    SOURCES
        session.cpp
    DEPENDS analysis config consumer periods record record_writers registry sources
            stream_writers window
    PRIVATE_DEPENDS collector environment sites)
