# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.

sushiprofiler_add_module(
    NAME live TIER session
    SOURCES
        closed_period.cpp
        live_channel.cpp
        live_view.cpp
        name_snapshot.cpp
        period_publisher.cpp
    DEPENDS version events record)
