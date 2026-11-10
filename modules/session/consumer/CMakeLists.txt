# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.

sushiprofiler_add_module(
    NAME consumer TIER session
    SOURCES
        consumer.cpp
        manual_poll_clock.cpp
        pause_log.cpp
        period_assembler.cpp
        period_retention.cpp
        steady_poll_clock.cpp
        stream_writer_set.cpp
    DEPENDS version clock collector fold live record sites sources stream_writers window)
