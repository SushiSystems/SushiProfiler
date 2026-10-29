# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

cmake_minimum_required(VERSION 3.21)

# An out-of-tree consumer of an INSTALLED SushiProfiler: find_package and nothing else. It builds
# one program against SushiProfiler::SushiProfiler and registers it as a test. `sp test --package`
# installs the build tree, builds this project against the install and runs the test.

project(SushiProfilerPackageConsumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The installed library is built under the release C runtime and no iterator checking in every
# build type (cmake/SushiProfilerAbiPinning.cmake); the consumer takes the same.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
set(CMAKE_MAP_IMPORTED_CONFIG_DEBUG Release RelWithDebInfo MinSizeRel None "")
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
    add_compile_definitions("$<$<COMPILE_LANGUAGE:CXX>:_ITERATOR_DEBUG_LEVEL=0>")
endif()

# The version is the one this tree builds, so the test follows each release without an edit.
file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/../../CMakeLists.txt" project_line
    REGEX "^project[(]SushiProfiler VERSION [0-9.]+")
string(REGEX MATCH "[0-9]+[.][0-9]+[.][0-9]+" expected_version "${project_line}")
find_package(SushiProfiler ${expected_version} EXACT REQUIRED)

# A consumer inherits no warning, sanitizer or policy target of SushiProfiler's own tree.
get_target_property(options SushiProfiler::SushiProfiler INTERFACE_COMPILE_OPTIONS)
get_target_property(links SushiProfiler::SushiProfiler INTERFACE_LINK_LIBRARIES)
foreach(leak IN ITEMS -Werror -fsanitize /WX sushiprofiler_warnings sushiprofiler_sanitizers)
    if("${options};${links}" MATCHES "${leak}")
        message(FATAL_ERROR "SushiProfiler::SushiProfiler hands its consumer '${leak}': "
            "${options};${links}")
    endif()
endforeach()

enable_testing()

add_executable(sushiprofiler_consumer consumer.cpp)
target_link_libraries(sushiprofiler_consumer PRIVATE SushiProfiler::SushiProfiler)
target_compile_definitions(sushiprofiler_consumer PRIVATE
    CONSUMER_EXPECTED_VERSION="${expected_version}")
add_test(NAME Small_Package.ReadsTheVersionThroughTheTarget COMMAND sushiprofiler_consumer)
set_tests_properties(Small_Package.ReadsTheVersionThroughTheTarget PROPERTIES
    LABELS small TIMEOUT 60)
