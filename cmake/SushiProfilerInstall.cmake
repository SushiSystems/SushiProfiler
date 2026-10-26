# SushiProfilerInstall.cmake
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

# Installs and exports the library target, its headers and the package files.

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(SP_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/SushiProfiler"
    CACHE STRING "Where the SushiProfiler CMake package files are installed")

# Installs sushiprofiler, every public include root, the export set and the package
# configuration. Call it after sushiprofiler_compose_library().
function(sushiprofiler_install_package)
    get_property(include_roots GLOBAL PROPERTY SUSHIPROFILER_INCLUDE_ROOTS)

    install(TARGETS sushiprofiler
        EXPORT SushiProfilerTargets
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

    foreach(include_root IN LISTS include_roots)
        install(DIRECTORY "${include_root}/"
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
            FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")
    endforeach()

    install(EXPORT SushiProfilerTargets
        FILE SushiProfilerTargets.cmake
        NAMESPACE SushiProfiler::
        DESTINATION "${SP_INSTALL_CMAKEDIR}")

    # The build tree is exported too, so a project that adds SushiProfiler as a subdirectory and
    # exports its own build tree finds SushiProfiler's targets in an export set.
    export(EXPORT SushiProfilerTargets
        FILE "${PROJECT_BINARY_DIR}/SushiProfilerTargets.cmake"
        NAMESPACE SushiProfiler::)

    configure_package_config_file(
        "${SP_CMAKE_DIR}/SushiProfilerConfig.cmake.in"
        "${PROJECT_BINARY_DIR}/SushiProfilerConfig.cmake"
        INSTALL_DESTINATION "${SP_INSTALL_CMAKEDIR}")

    # Before 1.0 a minor release may break the interface, so 0.1 is not handed 0.2.
    write_basic_package_version_file(
        "${PROJECT_BINARY_DIR}/SushiProfilerConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMinorVersion)

    install(FILES
        "${PROJECT_BINARY_DIR}/SushiProfilerConfig.cmake"
        "${PROJECT_BINARY_DIR}/SushiProfilerConfigVersion.cmake"
        DESTINATION "${SP_INSTALL_CMAKEDIR}")
endfunction()
