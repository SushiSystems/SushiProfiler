# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

cmake_minimum_required(VERSION 3.21)

set(SP_CMAKE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include("${SP_CMAKE_DIR}/SushiProfilerOptions.cmake")

project(SushiProfiler VERSION 0.3.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include("${SP_CMAKE_DIR}/SushiProfilerBuildPolicy.cmake")
include("${SP_CMAKE_DIR}/SushiProfilerRuntimeFiles.cmake")
include("${SP_CMAKE_DIR}/SushiProfilerModule.cmake")
include("${SP_CMAKE_DIR}/SushiProfilerLibrary.cmake")
include("${SP_CMAKE_DIR}/SushiProfilerInstall.cmake")
include("${SP_CMAKE_DIR}/SushiProfilerTests.cmake")

add_subdirectory(modules)
sushiprofiler_compose_library()

if(SP_INSTALL)
    sushiprofiler_install_package()
endif()

if(SP_BUILD_APPLICATIONS)
    add_subdirectory(applications)
endif()

if(SP_BUILD_TESTS)
    add_subdirectory(tests)
endif()
