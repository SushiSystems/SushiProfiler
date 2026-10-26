# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

# CMakeLists.txt

sushiprofiler_add_module(
    NAME umbrella TIER tools
    HEADER_ONLY
    DEPENDS version clock events sites rings collector zones lock_wait)
