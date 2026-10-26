# tests

The tests mirror `modules/`: the test of
`modules/<tier>/<module>/include/SushiProfiler/<module>/<name>.hpp` is
`tests/<tier>/<module>/test_<name>.cpp`. Each module's tests build one program,
`sp_<module>_tests`, through `sushiprofiler_add_module_tests` in `cmake/SushiProfilerTests.cmake`.

## Sizes

A test's size is the prefix of its GoogleTest suite. The build refuses a suite without one.

| Size | The test may use | Timeout |
| --- | --- | --- |
| `Small_` | One process and one thread, no sleep, no file outside the build tree | 60 s |
| `Medium_` | Threads, files in a temporary folder | 300 s |
| `Large_` | Real hardware, or a run longer than medium allows | 900 s |

`sp test --label small` runs one size; `sp test --package` also builds `tests/package/`, an
out-of-tree consumer of the installed package.
