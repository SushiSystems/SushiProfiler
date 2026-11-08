# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.

sushiprofiler_add_module(
    NAME environment TIER session
    SOURCES
        environment_contributor.cpp
        memory_contributor.cpp
        record_facts.cpp
    DEPENDS version record)
