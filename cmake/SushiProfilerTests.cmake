# SushiProfilerTests.cmake
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.

# One function builds the test program of a module. The tests of a module sit in
# tests/<tier>/<module>/, the mirror of its folder under modules/, and build one program there.
#
# A test's size is its GoogleTest suite-name prefix: Small_*, Medium_* and Large_*, registered
# with the label small, medium or large and a timeout of 60, 300 or 900 seconds per case. The cases
# of a program run in shards per size, <program>.<size>_shard_<i>of<N>. A test named in
# SP_QUARANTINED_TESTS keeps its size, gains the quarantined label and runs apart.
# `sp test --label <size>` runs one label.

# Suite.Case names of the intermittent tests, each with its known issue
# in docs/backlog/.
set(SP_QUARANTINED_TESTS "")

if(NOT SP_BUILD_TESTS)
    function(sushiprofiler_add_module_tests)
    endfunction()
    return()
endif()

enable_testing()

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

# The cases one CTest test runs, per size. A process pays its start-up (the runtime, the logger)
# once, so a case run alone is mostly start-up. A shard runs its share of a program's cases in
# one process, and the share follows the cost of a case of its size: a small case is a few
# milliseconds, so 300 of them are a few seconds and the run still spreads over every core; a
# medium case uses threads or files and takes up to 5 s, so 25 keep a shard well under a minute;
# a large case takes up to 120 s and runs alone.
set(SP_SHARD_CASES_SMALL 300)
set(SP_SHARD_CASES_MEDIUM 25)
set(SP_SHARD_CASES_LARGE 1)

# Where each shard writes its per-case GoogleTest XML; the time reader reads the folder, so the
# per-case limits of the testing skill still apply to a case run inside a shard.
set(SP_GTEST_RESULTS_DIR "${PROJECT_BINARY_DIR}/test_results/gtest")

# Counts the cases of <sources> whose suite opens with <prefix> and sets <out_var> to the count.
# A TEST_P or TYPED_TEST_P counts once however many instances it gets, so the count is a floor.
# The suite prefix names the size for TEST_P too, because the suite name of a TEST_P carries it
# and its instantiation is named Small_<x>/<suite>.<case>/<n>.
function(sushiprofiler_count_cases out_var prefix)
    set(count 0)
    foreach(source IN LISTS ARGN)
        file(READ "${source}" text)
        string(REGEX MATCHALL "(TEST|TEST_F|TEST_P|TYPED_TEST|TYPED_TEST_P)[ \t]*[(][ \t\r\n]*${prefix}[A-Za-z0-9_]*"
            declarations "${text}")
        list(LENGTH declarations declared)
        math(EXPR count "${count} + ${declared}")
    endforeach()
    set(${out_var} ${count} PARENT_SCOPE)
endfunction()

# Registers one CTest test that runs <program> with <filter>, as shard <index> of <shards>.
# GoogleTest splits the filtered cases round-robin by GTEST_TOTAL_SHARDS and GTEST_SHARD_INDEX,
# and the test writes its per-case XML under SP_GTEST_RESULTS_DIR. GTEST_SHUFFLE and
# GTEST_RANDOM_SEED of the calling environment reach it.
function(sushiprofiler_add_shard_test program name filter labels timeout index shards)
    add_test(NAME ${name}
        COMMAND ${program} --gtest_filter=${filter}
            --gtest_output=xml:${SP_GTEST_RESULTS_DIR}/${name}.xml)
    set_tests_properties(${name} PROPERTIES
        LABELS "${labels}"
        TIMEOUT ${timeout}
        ENVIRONMENT "GTEST_TOTAL_SHARDS=${shards};GTEST_SHARD_INDEX=${index}")
    # A large shard runs on the real accelerator, of which the resource file CTest gets holds one
    # slot each, so two large shards never share one.
    if("large" IN_LIST labels)
        set_tests_properties(${name} PROPERTIES RESOURCE_GROUPS "gpus:1")
    endif()
endfunction()

# Registers the cases of <program> of one size in shards, with the size's label and a timeout of
# the case timeout times the cases of a shard. The quarantined cases of the size run apart in one
# test that also carries `quarantined`, and a GoogleTest filter after '-' leaves them out of the
# shards. <size_group> is "<prefix>|<label>|<case timeout>".
function(sushiprofiler_register_size_shards program module size_group)
    string(REPLACE "|" ";" parts "${size_group}")
    list(GET parts 0 prefix)
    list(GET parts 1 label)
    list(GET parts 2 case_timeout)
    string(TOUPPER "${label}" upper)
    sushiprofiler_count_cases(cases "${prefix}" ${ARGN})

    # An instantiated suite is named <instance>/<suite>, so each pattern also carries the slash.
    set(quarantined "")
    set(quarantined_cases 0)
    foreach(name IN LISTS SP_QUARANTINED_TESTS)
        if(name MATCHES "^${prefix}")
            list(APPEND quarantined "${name}" "*/${name}")
            math(EXPR quarantined_cases "${quarantined_cases} + 1")
        endif()
    endforeach()
    set(filter "${prefix}*:*/${prefix}*")
    if(quarantined)
        list(JOIN quarantined ":" quarantined_filter)
        string(APPEND filter "-${quarantined_filter}")
        math(EXPR quarantined_timeout "${case_timeout} * ${quarantined_cases}")
        sushiprofiler_add_shard_test(${program} ${program}.${label}_quarantined
            "${quarantined_filter}" "${label};${module};quarantined" ${quarantined_timeout} 0 1)
        math(EXPR cases "${cases} - ${quarantined_cases}")
    endif()
    if(cases LESS 1)
        return()
    endif()

    math(EXPR shards "(${cases} + ${SP_SHARD_CASES_${upper}} - 1) / ${SP_SHARD_CASES_${upper}}")
    math(EXPR shard_cases "(${cases} + ${shards} - 1) / ${shards}")
    math(EXPR timeout "${case_timeout} * ${shard_cases}")
    math(EXPR last "${shards} - 1")
    foreach(index RANGE ${last})
        sushiprofiler_add_shard_test(${program} ${program}.${label}_shard_${index}of${shards}
            "${filter}" "${label};${module}" ${timeout} ${index} ${shards})
    endforeach()
endfunction()

# Registers the tests of <program> in shards, one block per size: Small_, Medium_ and Large_ with
# the label small, medium or large and a timeout per case of 60, 300 or 900 seconds. The cases are
# counted in the source files that follow the module name, given by absolute path. This is the one
# function that registers shards.
function(sushiprofiler_register_sized_tests program module)
    set(groups "Small_|small|60" "Medium_|medium|300" "Large_|large|900")
    foreach(group IN LISTS groups)
        sushiprofiler_register_size_shards(${program} ${module} "${group}" ${ARGN})
    endforeach()
endfunction()

# Builds the benchmark program of <source>, named sp_bench_<name> for benchmark_<name>.cpp, into
# benchmarks/<config>/ of the build tree. It carries the label benchmark and is never registered
# with ctest, because a benchmark measures and decides nothing.
function(sushiprofiler_add_benchmark source)
    get_filename_component(file_name "${source}" NAME_WE)
    if(NOT file_name MATCHES "^benchmark_(.+)$")
        message(FATAL_ERROR "sushiprofiler_add_benchmark(): '${source}' is not benchmark_<name>.cpp.")
    endif()
    set(program "sp_bench_${CMAKE_MATCH_1}")
    add_executable(${program} "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
    target_link_libraries(${program} PRIVATE SushiProfiler::SushiProfiler)
    target_include_directories(${program} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    sushiprofiler_apply_build_policy(${program})
    sushiprofiler_copy_runtime_files(${program})
    set_target_properties(${program} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/benchmarks/$<CONFIG>"
        LABELS benchmark)
endfunction()

# sushiprofiler_add_module_tests(MODULE <module> SOURCES <file>... [BENCHMARKS <file>...])
#
# Called from tests/<tier>/<module>/CMakeLists.txt, with paths relative to that folder. Builds
# sp_<module>_tests against SushiProfiler::SushiProfiler with tests/ on its include path, so a test
# reaches tests/support/ as "support/<name>.hpp", copies the DLL beside it, refuses a suite with
# no size prefix, and registers the cases of every size in shards. With SP_BUILD_BENCHMARKS on, each
# BENCHMARKS file becomes a benchmark program.
function(sushiprofiler_add_module_tests)
    cmake_parse_arguments(ARG "" "MODULE" "SOURCES;BENCHMARKS" ${ARGN})
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
    target_include_directories(${program} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/tests")
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

    sushiprofiler_register_sized_tests(${program} ${ARG_MODULE} ${sources})

    if(SP_BUILD_BENCHMARKS)
        foreach(source IN LISTS ARG_BENCHMARKS)
            sushiprofiler_add_benchmark("${source}")
        endforeach()
    endif()
endfunction()
