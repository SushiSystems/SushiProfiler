# SushiProfilerRuntimeFiles.cmake
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

# Copies the shared libraries an executable loads beside it, so it starts without a search path.

# sushiprofiler_copy_runtime_files(<target>)
#
# On Windows, copies every DLL in $<TARGET_RUNTIME_DLLS:target> into the target's folder after
# each link. Elsewhere the build tree's RPATH finds the shared library, and nothing is copied.
# A target that loads no DLL runs no command, since copy_if_different refuses an empty list.
function(sushiprofiler_copy_runtime_files target)
    if(NOT WIN32)
        return()
    endif()
    set(dlls "$<TARGET_RUNTIME_DLLS:${target}>")
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND "$<$<BOOL:${dlls}>:${CMAKE_COMMAND};-E;copy_if_different;${dlls};$<TARGET_FILE_DIR:${target}>>"
        COMMAND_EXPAND_LISTS
        VERBATIM)
endfunction()
