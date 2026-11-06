# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.

# Adds the test folder of every module, the mirror of modules/. tests/package/ is a project of
# its own, built against the installed package.

sushiprofiler_module_folders(folders)
foreach(folder IN LISTS folders)
    add_subdirectory(${folder})
endforeach()
