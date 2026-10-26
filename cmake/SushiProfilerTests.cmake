# SushiProfilerTests.cmake
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

# One function builds the test program of a module. The tests of a module sit in
# tests/<tier>/<module>/, the mirror of its folder under modules/, and build one program there.
#
# A test's size is its GoogleTest suite-name prefix: Small_*, Medium_* and Large_*, registered
# with the label small, medium or large and a timeout of 60, 300 or 900 seconds. A test named
# in SP_QUARANTINED_TESTS keeps its size and gains the quarantined label.
# `sp test --label <size>` runs one label.

# Suite.Case names of the intermittent tests, each with its entry in
# docs/reference/KNOWN_ISSUES.md.
set(SP_QUARANTINED_TESTS "")

if(NOT SP_BUILD_TESTS)
    function(sushiprofiler_add_module_tests)
    endfunction()
    return()
endif()

enable_testing()
include(GoogleTest)

# The pinned release is built with the tests, so it shares their configuration and runtime.
if(NOT TARGET GTest::gtest)
    include(FetchContent)
    FetchContent_Declare(googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
        URL_HASH SHA256=8ad598c73ad796e0d8280b082cebd82a630d73e73cd3c70057938a6501bba5d7
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# Registers the tests of <program> whose suite opens with each size prefix.
function(sushiprofiler_register_sized_tests program module)
    set(groups "Small_|small|60" "Medium_|medium|300" "Large_|large|900")
    foreach(group IN LISTS groups)
        string(REPLACE "|" ";" parts "${group}")
        list(GET parts 0 prefix)
        list(GET parts 1 label)
        list(GET parts 2 timeout)

        set(quarantined "")
        foreach(name IN LISTS SP_QUARANTINED_TESTS)
            if(name MATCHES "^${prefix}")
                list(APPEND quarantined "${name}")
            endif()
        endforeach()
        list(JOIN quarantined ":" quarantined_filter)

        set(filter "${prefix}*")
        if(quarantined_filter)
            string(APPEND filter "-${quarantined_filter}")
            gtest_discover_tests(${program}
                DISCOVERY_MODE PRE_TEST
                TEST_FILTER "${quarantined_filter}"
                PROPERTIES LABELS "${label}\;${module}\;quarantined" TIMEOUT ${timeout})
        endif()
        gtest_discover_tests(${program}
            DISCOVERY_MODE PRE_TEST
            TEST_FILTER "${filter}"
            PROPERTIES LABELS "${label}\;${module}" TIMEOUT ${timeout})
    endforeach()
endfunction()

# sushiprofiler_add_module_tests(MODULE <module> SOURCES <file>...)
#
# Called from tests/<tier>/<module>/CMakeLists.txt, with paths relative to that folder. Builds
# sp_<module>_tests against SushiProfiler::SushiProfiler, copies the DLL beside it, refuses a suite with
# no size prefix, and registers every test with its size.
function(sushiprofiler_add_module_tests)
    cmake_parse_arguments(ARG "" "MODULE" "SOURCES" ${ARGN})
    if("${ARG_MODULE}" STREQUAL "" OR NOT ARG_SOURCES)
        message(FATAL_ERROR "sushiprofiler_add_module_tests(): MODULE and SOURCES are required.")
    endif()
    set(program "sp_${ARG_MODULE}_tests")
    set(sources "")
    foreach(source IN LISTS ARG_SOURCES)
        list(APPEND sources "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
    endforeach()

    add_executable(${program} ${sources})
    target_link_libraries(${program} PRIVATE SushiProfiler::SushiProfiler GTest::gtest_main)
    target_include_directories(${program} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    sushiprofiler_apply_build_policy(${program})
    sushiprofiler_copy_runtime_files(${program})

    # A suite with no size prefix would register no test, so the build refuses it.
    set(stamp "${CMAKE_CURRENT_BINARY_DIR}/${program}_sizes.stamp")
    list(JOIN sources "|" joined_sources)
    add_custom_command(
        OUTPUT "${stamp}"
        COMMAND ${CMAKE_COMMAND} "-DSOURCES=${joined_sources}" "-DSTAMP=${stamp}"
            -P "${SP_CMAKE_DIR}/CheckTestSizes.cmake"
        DEPENDS ${sources} "${SP_CMAKE_DIR}/CheckTestSizes.cmake"
        COMMENT "Checking the test sizes of ${program}"
        VERBATIM)
    add_custom_target(${program}_sizes DEPENDS "${stamp}")
    add_dependencies(${program} ${program}_sizes)

    sushiprofiler_register_sized_tests(${program} ${ARG_MODULE})
endfunction()
