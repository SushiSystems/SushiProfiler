# SushiProfilerLibrary.cmake
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

# Composes the module include roots and sources into the one library target,
# SushiProfiler::SushiProfiler. While no module has a source file it is an INTERFACE library that
# carries the include roots and Threads; once a module lists SOURCES it is a static library over
# them.

function(sushiprofiler_compose_library)
    get_property(sources       GLOBAL PROPERTY SUSHIPROFILER_SOURCES)
    get_property(include_roots GLOBAL PROPERTY SUSHIPROFILER_INCLUDE_ROOTS)
    get_property(source_roots  GLOBAL PROPERTY SUSHIPROFILER_SOURCE_ROOTS)
    find_package(Threads REQUIRED)

    if(sources)
        add_library(sushiprofiler STATIC ${sources})
        set(scope PUBLIC)
        target_include_directories(sushiprofiler PRIVATE ${source_roots})
        set_target_properties(sushiprofiler PROPERTIES
            CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON POSITION_INDEPENDENT_CODE ON)
        sushiprofiler_apply_build_policy(sushiprofiler)
    else()
        add_library(sushiprofiler INTERFACE)
        set(scope INTERFACE)
    endif()
    add_library(SushiProfiler::SushiProfiler ALIAS sushiprofiler)
    foreach(include_root IN LISTS include_roots)
        target_include_directories(sushiprofiler ${scope} "$<BUILD_INTERFACE:${include_root}>")
    endforeach()
    target_include_directories(sushiprofiler ${scope}
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
    target_link_libraries(sushiprofiler ${scope} Threads::Threads)
    target_compile_features(sushiprofiler ${scope} cxx_std_17)
    set_target_properties(sushiprofiler PROPERTIES EXPORT_NAME SushiProfiler)
endfunction()
