# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.

sushiprofiler_add_module_tests(
    MODULE live
    SOURCES
        test_closed_period.cpp
        test_live_channel.cpp
        test_live_view.cpp
        test_name_snapshot.cpp
        test_period_publisher.cpp)
