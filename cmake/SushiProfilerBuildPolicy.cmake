# SushiProfilerBuildPolicy.cmake
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

# The compile policy of this tree: warnings and the sanitizer SP_SANITIZE names. Both reach the
# targets of this tree only; an installed consumer inherits neither.

add_library(sushiprofiler_warnings INTERFACE)
if(MSVC)
    target_compile_options(sushiprofiler_warnings INTERFACE /W4 /WX /permissive- /EHsc)
    target_compile_definitions(sushiprofiler_warnings INTERFACE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(sushiprofiler_warnings INTERFACE -Wall -Wextra -Wpedantic -Werror)
endif()

add_library(sushiprofiler_sanitizers INTERFACE)
if(SP_SANITIZE STREQUAL "address")
    if(MSVC)
        target_compile_options(sushiprofiler_sanitizers INTERFACE /fsanitize=address)
    else()
        target_compile_options(sushiprofiler_sanitizers INTERFACE
            -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
        target_link_options(sushiprofiler_sanitizers INTERFACE -fsanitize=address,undefined)
    endif()
elseif(SP_SANITIZE STREQUAL "thread")
    if(MSVC)
        message(FATAL_ERROR "SP_SANITIZE=thread: MSVC has no ThreadSanitizer; use clang or gcc.")
    endif()
    target_compile_options(sushiprofiler_sanitizers INTERFACE -fsanitize=thread)
    target_link_options(sushiprofiler_sanitizers INTERFACE -fsanitize=thread)
elseif(NOT SP_SANITIZE STREQUAL "")
    message(FATAL_ERROR "SP_SANITIZE='${SP_SANITIZE}': use address, thread or nothing.")
endif()

# Applies the warnings and the sanitizer to one target of this tree. Their settings are copied
# onto the target rather than linked, because a static library carries even a private link into
# its link interface, and a build-tree export would then require both names in its export set.
function(sushiprofiler_apply_build_policy target)
    get_target_property(type ${target} TYPE)
    foreach(policy IN ITEMS sushiprofiler_warnings sushiprofiler_sanitizers)
        target_compile_options(${target} PRIVATE
            "$<TARGET_PROPERTY:${policy},INTERFACE_COMPILE_OPTIONS>")
        target_compile_definitions(${target} PRIVATE
            "$<TARGET_PROPERTY:${policy},INTERFACE_COMPILE_DEFINITIONS>")
        if(NOT type STREQUAL "STATIC_LIBRARY")
            target_link_options(${target} PRIVATE
                "$<TARGET_PROPERTY:${policy},INTERFACE_LINK_OPTIONS>")
        endif()
    endforeach()
endfunction()
