# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.

sushiprofiler_add_module(
    NAME sites TIER capture
    SOURCES
        dynamic_site_table.cpp
        name_table.cpp
        site_registry.cpp
    DEPENDS version events)
