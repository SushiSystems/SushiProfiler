# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

# CMakeLists.txt

# The version header holds the number the library reports; project() holds the one CMake
# packages. The two must agree, so a release that moves one without the other stops here.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/include/SushiProfiler/version/version.hpp"
    version_line REGEX "K_VERSION_STRING")
string(REGEX MATCH "\"([0-9]+[.][0-9]+[.][0-9]+)\"" version_unused "${version_line}")
if(NOT CMAKE_MATCH_1 STREQUAL PROJECT_VERSION)
    message(FATAL_ERROR
        "version.hpp declares '${CMAKE_MATCH_1}'; project() in CMakeLists.txt declares "
        "'${PROJECT_VERSION}'.")
endif()

sushiprofiler_add_module(
    NAME version TIER foundation
    HEADER_ONLY)
