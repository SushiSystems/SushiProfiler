# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

# sp_record: show, compare, check and validate run records; `sp record` runs it.
add_executable(sp_record main.cpp)
target_link_libraries(sp_record PRIVATE SushiProfiler::SushiProfiler)
sushiprofiler_apply_build_policy(sp_record)
sushiprofiler_copy_runtime_files(sp_record)
set_target_properties(sp_record PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/applications/$<CONFIG>")
