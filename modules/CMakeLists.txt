# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

# Adds every module folder in the order cmake/SushiProfilerTiers.cmake states.

sushiprofiler_module_folders(folders)
foreach(folder IN LISTS folders)
    add_subdirectory(${folder})
endforeach()
