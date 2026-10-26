# SushiProfilerLibrary.cmake
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

# Composes the module include roots and sources into two library targets:
#   sushiprofiler         SushiProfiler::SushiProfiler, shared, so every image of a process
#                         reaches one collector.
#   sushiprofiler_static  SushiProfiler::SushiProfilerStatic, static with hidden symbols, for a
#                         plug-in that must keep a collector of its own.

# Declares one library target over the recorded sources.
function(sushiprofiler_add_library_target target kind export_name)
    get_property(sources       GLOBAL PROPERTY SUSHIPROFILER_SOURCES)
    get_property(include_roots GLOBAL PROPERTY SUSHIPROFILER_INCLUDE_ROOTS)
    get_property(source_roots  GLOBAL PROPERTY SUSHIPROFILER_SOURCE_ROOTS)
    find_package(Threads REQUIRED)

    add_library(${target} ${kind} ${sources})
    add_library(SushiProfiler::${export_name} ALIAS ${target})
    foreach(include_root IN LISTS include_roots)
        target_include_directories(${target} PUBLIC "$<BUILD_INTERFACE:${include_root}>")
    endforeach()
    target_include_directories(${target} PUBLIC "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
    target_include_directories(${target} PRIVATE ${source_roots})
    target_link_libraries(${target} PUBLIC Threads::Threads)
    target_compile_features(${target} PUBLIC cxx_std_17)
    sushiprofiler_apply_build_policy(${target})
    set_target_properties(${target} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        POSITION_INDEPENDENT_CODE ON
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
        EXPORT_NAME ${export_name})
endfunction()

function(sushiprofiler_compose_library)
    # Before 1.0 a minor release may break the ABI, so the minor number is part of the name.
    if(PROJECT_VERSION_MAJOR EQUAL 0)
        set(soversion "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
    else()
        set(soversion "${PROJECT_VERSION_MAJOR}")
    endif()

    sushiprofiler_add_library_target(sushiprofiler SHARED SushiProfiler)
    target_compile_definitions(sushiprofiler PRIVATE SUSHIPROFILER_EXPORTS)
    set_target_properties(sushiprofiler PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${soversion}
        OUTPUT_NAME sushiprofiler)

    sushiprofiler_add_library_target(sushiprofiler_static STATIC SushiProfilerStatic)
    target_compile_definitions(sushiprofiler_static PUBLIC SUSHIPROFILER_STATIC)
    set_target_properties(sushiprofiler_static PROPERTIES OUTPUT_NAME sushiprofiler_static)
endfunction()
