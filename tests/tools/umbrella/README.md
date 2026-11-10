# tests/tools/umbrella

The check of `SushiProfiler/SushiProfiler.hpp`: `compile_SushiProfiler.cpp` includes the umbrella header
alone and `static_assert`s one public name of each of the json, events, collector, config and record
modules and the version. The header has no runtime promise, so no test runs; the build compiles the file.
