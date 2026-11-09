# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.

sushiprofiler_add_module_tests(
    MODULE sites
    SOURCES
        compile_site.cpp
        test_dynamic_site_table.cpp
        test_name_table.cpp
        test_site_registry.cpp)
