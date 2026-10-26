# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

sushiprofiler_add_module_tests(
    MODULE collector
    SOURCES
        test_collector.cpp
        test_collector_settings.cpp
        test_drain_batch.cpp)

# A second shared image that records a zone, so a test proves both images reach one collector.
add_library(sp_second_image SHARED second_image.cpp)
target_compile_definitions(sp_second_image PRIVATE SP_SECOND_IMAGE_EXPORTS)
target_link_libraries(sp_second_image PRIVATE SushiProfiler::SushiProfiler)
target_include_directories(sp_second_image PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
sushiprofiler_apply_build_policy(sp_second_image)
target_link_libraries(sp_collector_tests PRIVATE sp_second_image)
