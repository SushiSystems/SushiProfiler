# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.

# Adds every module folder in the order cmake/SushiProfilerTiers.cmake states.

sushiprofiler_module_folders(folders)
foreach(folder IN LISTS folders)
    add_subdirectory(${folder})
endforeach()
