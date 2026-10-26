# version

`SushiProfiler/version/version.hpp` declares the library version: `K_VERSION_MAJOR`,
`K_VERSION_MINOR`, `K_VERSION_PATCH` and `K_VERSION_STRING`, in the namespace `SushiProfiler`.
`SushiProfiler/version/export.hpp` defines `SUSHIPROFILER_API`, which marks what the shared
library exports; under `SUSHIPROFILER_STATIC`, which the static target sets, it is empty. The
module is header-only and depends on the standard library alone.

`CMakeLists.txt` of this module compares `K_VERSION_STRING` with the version `project()` declares
in the root `CMakeLists.txt` and stops the configure step when they differ.

The test is in `tests/foundation/version/`.
