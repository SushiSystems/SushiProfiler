# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.

sushiprofiler_add_module_tests(
    MODULE consumer
    SOURCES
        test_consumer.cpp
        test_manual_poll_clock.cpp
        test_pause_log.cpp
        test_period_assembler.cpp
        test_period_retention.cpp
        test_poll_clock.cpp
        test_steady_poll_clock.cpp
        test_stream_writer_set.cpp
    BENCHMARKS
        benchmark_consumer.cpp)
