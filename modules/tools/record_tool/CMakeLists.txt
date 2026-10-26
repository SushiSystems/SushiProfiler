# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

sushiprofiler_add_module(
    NAME record_tool TIER tools
    SOURCES
        check_command.cpp
        compare_command.cpp
        record_tool.cpp
        show_command.cpp
        tool_arguments.cpp
        validate_command.cpp
    PRIVATE_DEPENDS analysis checks compare record record_json record_writers)
