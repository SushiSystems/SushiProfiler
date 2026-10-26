# SushiProfilerModule.cmake
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

# One function declares a module. A module is an INTERFACE target carrying its include root
# and those of the modules it depends on; its sources are recorded for the library targets that
# cmake/SushiProfilerLibrary.cmake composes, and each of its public headers is compiled alone.

include("${CMAKE_CURRENT_LIST_DIR}/SushiProfilerTiers.cmake")

# Refuses a dependency the tier table does not allow: one above its consumer, or one beside it
# that SUSHIPROFILER_SAME_TIER_EDGES does not list. An umbrella module includes its scope.
function(sushiprofiler_check_module_edge consumer dependency)
    sushiprofiler_module_tier("${dependency}" dependency_tier)
    if(dependency_tier STREQUAL "")
        message(FATAL_ERROR
            "sushiprofiler_add_module(${consumer}): depends on '${dependency}', which "
            "SUSHIPROFILER_MODULE_TIERS (cmake/SushiProfilerTiers.cmake) does not list.")
    endif()
    if(NOT TARGET sushiprofiler_module_${dependency})
        message(FATAL_ERROR
            "sushiprofiler_add_module(${consumer}): '${dependency}' is not declared yet; a "
            "dependency comes before its consumer in SUSHIPROFILER_MODULE_TIERS.")
    endif()
    if(consumer STREQUAL "umbrella")
        return()
    endif()
    sushiprofiler_module_tier("${consumer}" consumer_tier)
    list(FIND SUSHIPROFILER_TIER_ORDER "${consumer_tier}" consumer_index)
    list(FIND SUSHIPROFILER_TIER_ORDER "${dependency_tier}" dependency_index)
    if(dependency_index GREATER consumer_index)
        message(FATAL_ERROR
            "sushiprofiler_add_module(${consumer}): ${dependency} (${dependency_tier}) is above "
            "${consumer} (${consumer_tier}) in SUSHIPROFILER_TIER_ORDER.")
    endif()
    if(dependency_index EQUAL consumer_index)
        sushiprofiler_same_tier_edge_allowed("${consumer}" "${dependency}" allowed)
        if(NOT allowed)
            message(FATAL_ERROR
                "sushiprofiler_add_module(${consumer}): undeclared edge ${consumer} -> "
                "${dependency} inside the ${consumer_tier} tier. List it in "
                "SUSHIPROFILER_SAME_TIER_EDGES (cmake/SushiProfilerTiers.cmake) or move the "
                "shared code down a tier.")
        endif()
    endif()
endfunction()

# Compiles every public header of a module alone, with its declared include roots only, so a
# header that forgot one of its own includes fails here.
function(sushiprofiler_add_header_check module include_root)
    file(GLOB_RECURSE headers CONFIGURE_DEPENDS RELATIVE "${include_root}"
        "${include_root}/*.hpp" "${include_root}/*.h")
    set(units "")
    foreach(header IN LISTS headers)
        string(MAKE_C_IDENTIFIER "${header}" unit_name)
        set(unit "${CMAKE_CURRENT_BINARY_DIR}/header_check/${unit_name}.cpp")
        file(CONFIGURE OUTPUT "${unit}" CONTENT "#include <${header}>\n" @ONLY)
        list(APPEND units "${unit}")
    endforeach()
    if(NOT units)
        return()
    endif()
    set(check "sushiprofiler_module_${module}_header_check")
    add_library(${check} OBJECT ${units})
    target_link_libraries(${check} PRIVATE "sushiprofiler_module_${module}")
    sushiprofiler_apply_build_policy(${check})
endfunction()

# sushiprofiler_add_module(NAME <module> TIER <tier> [HEADER_ONLY]
#                     [SOURCES <file under source/>...] [DEPENDS <module>...]
#                     [PRIVATE_DEPENDS <module>...])
function(sushiprofiler_add_module)
    cmake_parse_arguments(ARG "HEADER_ONLY" "NAME;TIER" "SOURCES;DEPENDS;PRIVATE_DEPENDS" ${ARGN})
    if("${ARG_NAME}" STREQUAL "" OR "${ARG_TIER}" STREQUAL "")
        message(FATAL_ERROR "sushiprofiler_add_module(): NAME and TIER are required.")
    endif()
    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "sushiprofiler_add_module(${ARG_NAME}): unexpected '${ARG_UNPARSED_ARGUMENTS}'.")
    endif()
    sushiprofiler_module_tier("${ARG_NAME}" table_tier)
    if(NOT "${ARG_TIER}" STREQUAL "${table_tier}")
        message(FATAL_ERROR
            "sushiprofiler_add_module(${ARG_NAME}): declares TIER '${ARG_TIER}'; "
            "cmake/SushiProfilerTiers.cmake says '${table_tier}'.")
    endif()
    if(ARG_HEADER_ONLY AND ARG_SOURCES)
        message(FATAL_ERROR
            "sushiprofiler_add_module(${ARG_NAME}): HEADER_ONLY and SOURCES exclude each other.")
    endif()
    if(NOT ARG_HEADER_ONLY AND NOT ARG_SOURCES)
        message(FATAL_ERROR "sushiprofiler_add_module(${ARG_NAME}): no SOURCES; mark it HEADER_ONLY.")
    endif()
    if(ARG_HEADER_ONLY AND ARG_PRIVATE_DEPENDS)
        message(FATAL_ERROR
            "sushiprofiler_add_module(${ARG_NAME}): a HEADER_ONLY module has no PRIVATE_DEPENDS.")
    endif()

    set(target "sushiprofiler_module_${ARG_NAME}")
    set(include_root "${CMAKE_CURRENT_SOURCE_DIR}/include")
    add_library(${target} INTERFACE)
    target_include_directories(${target} INTERFACE "$<BUILD_INTERFACE:${include_root}>")
    foreach(dependency IN LISTS ARG_DEPENDS)
        sushiprofiler_check_module_edge("${ARG_NAME}" "${dependency}")
        target_link_libraries(${target} INTERFACE "sushiprofiler_module_${dependency}")
    endforeach()
    # The library compiles every source with every include root; a private edge is only checked.
    foreach(dependency IN LISTS ARG_PRIVATE_DEPENDS)
        sushiprofiler_check_module_edge("${ARG_NAME}" "${dependency}")
    endforeach()
    set_property(GLOBAL APPEND PROPERTY SUSHIPROFILER_INCLUDE_ROOTS "${include_root}")

    foreach(source IN LISTS ARG_SOURCES)
        set_property(GLOBAL APPEND PROPERTY SUSHIPROFILER_SOURCES
            "${CMAKE_CURRENT_SOURCE_DIR}/source/${source}")
    endforeach()
    if(ARG_SOURCES)
        set_property(GLOBAL APPEND PROPERTY SUSHIPROFILER_SOURCE_ROOTS
            "${CMAKE_CURRENT_SOURCE_DIR}/source")
    endif()

    sushiprofiler_add_header_check("${ARG_NAME}" "${include_root}")
endfunction()
