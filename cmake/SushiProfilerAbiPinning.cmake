# SushiProfilerAbiPinning.cmake
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

# The C runtime and the iterator checking of every build type, and the debug information of Debug.
# They are set at directory scope before any target exists, so the library, the tests and the
# fetched GoogleTest compile under the same ones.

# Every build type takes the release C runtime, so a Debug library links the Release packages
# beneath it and shares their heap. Memory errors are the sanitizer tree's job. Ignored off the
# MSVC ABI.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

# A Debug consumer of an imported target links its Release build, which shares that runtime.
set(CMAKE_MAP_IMPORTED_CONFIG_DEBUG Release RelWithDebInfo MinSizeRel "")

# The MSVC STL's container layout depends on this; libstdc++ and libc++ never read it.
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
    add_compile_definitions("$<$<COMPILE_LANGUAGE:CXX>:_ITERATOR_DEBUG_LEVEL=0>")
endif()

# Debug host code keeps line tables only: a stack trace gets its files and lines at a fraction of
# full debug information's compile memory, object size and link time. CMake states -g for Debug;
# the later -gline-tables-only overrides it. RelWithDebInfo carries full debug information.
if(NOT CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    add_compile_options("$<$<AND:$<CONFIG:Debug>,$<COMPILE_LANGUAGE:CXX>>:-gline-tables-only>")
endif()
