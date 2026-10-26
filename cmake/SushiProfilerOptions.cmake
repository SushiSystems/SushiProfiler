# SushiProfilerOptions.cmake
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

# Declares every option a person or the CLI sets. Included before project().

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(SP_IS_TOP_LEVEL ON)
else()
    set(SP_IS_TOP_LEVEL OFF)
endif()

option(SP_BUILD_TESTS "Build the test programs" ${SP_IS_TOP_LEVEL})
option(SP_BUILD_BENCHMARKS "Build the benchmark programs beside the tests" OFF)
option(SP_BUILD_APPLICATIONS "Build the programs under applications/" ${SP_IS_TOP_LEVEL})
option(SP_INSTALL "Add the install rules and the package files" ON)

# address: AddressSanitizer with UndefinedBehaviorSanitizer; thread: ThreadSanitizer.
set(SP_SANITIZE "" CACHE STRING "Sanitizer build: empty, address or thread")
set_property(CACHE SP_SANITIZE PROPERTY STRINGS "" address thread)
