# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

sushiprofiler_add_module_tests(
    MODULE live
    SOURCES
        test_closed_period.cpp
        test_live_channel.cpp
        test_live_view.cpp
        test_name_snapshot.cpp
        test_period_publisher.cpp)
