# tests/package

An out-of-tree consumer of an installed SushiProfiler. It finds the package with `find_package`
and nothing else, refuses a target that hands it SushiProfiler's warnings or sanitizer, and
builds `consumer.cpp` against `SushiProfiler::SushiProfiler`. The program includes the umbrella
header and fails unless the version it reads equals the one the tree builds.

`sp test --package` installs the build tree under `build/package-prefix/`, builds this project
in `build/package/` against it and runs the program as a test.
