# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.

# CMakeLists.txt

sushiprofiler_add_module(
    NAME umbrella TIER tools
    HEADER_ONLY
    DEPENDS version clock events sites rings collector zones lock_wait periods json record record_json statistics analysis record_writers compare checks config window sources environment fold stream_writers registry consumer session record_tool)
