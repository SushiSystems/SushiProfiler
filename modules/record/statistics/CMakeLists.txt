# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.

sushiprofiler_add_module(
    NAME statistics TIER record
    SOURCES
        bootstrap.cpp
        dispersion.cpp
        log2_histogram.cpp
        sorted_samples.cpp
        steady_state.cpp
    DEPENDS version)
