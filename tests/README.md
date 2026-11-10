# tests

The tests mirror `modules/`: the test of
`modules/<tier>/<module>/include/SushiProfiler/<module>/<name>.hpp` is
`tests/<tier>/<module>/test_<name>.cpp`. Each module's tests build one program,
`sp_<module>_tests`, through `sushiprofiler_add_module_tests` in `cmake/SushiProfilerTests.cmake`.

`tests/support/` holds what the tests of several modules share, reached as
`#include "support/<name>.hpp"`. `tolerance.hpp` is the copy of SushiSkills's
`tools/testing/support/tolerance.hpp`: every comparison with a tolerance derives its bound there.

## Sizes

A test's size is the prefix of its GoogleTest suite. A suite without one is never registered with ctest, and `tools/testing/check_test_sizes.py` fails on it. A `TEST_P` suite carries the prefix in its own name and in the name of its instantiation, `Small_<x>`.

| Size | The test may use | Timeout |
| --- | --- | --- |
| `Small_` | One process and one thread, no sleep, no file outside the build tree | 60 s |
| `Medium_` | Threads, files in a temporary folder | 300 s |
| `Large_` | Real hardware, or a run longer than medium allows | 900 s |

The timeout is per case. A case named in `SP_QUARANTINED_TESTS` runs in a test of its own with the
label `quarantined` beside its size.

### Shards

A case run in a process of its own pays the process and runtime start-up, which is most of a small
case. `cmake/SushiProfilerTests.cmake` registers the cases of a program in shards instead: per
program and per size, N CTest tests named `<program>.<size>_shard_<i>of<N>`, each running the
program with `--gtest_filter=<prefix>*:*/<prefix>*` (without the quarantined cases),
`GTEST_TOTAL_SHARDS=N` and `GTEST_SHARD_INDEX=i`. GoogleTest splits the filtered cases between the
shards, so every case runs in exactly one. N is the case count of the size divided by the cases one
shard holds, rounded up: `SP_SHARD_CASES_SMALL` (300), `_MEDIUM` (25) and `_LARGE` (1). The count
comes from the `TEST`, `TEST_F`, `TEST_P`, `TYPED_TEST` and `TYPED_TEST_P` declarations in the
program's sources, so a parametrised suite counts once and N is a floor. A shard's timeout is the
size's timeout times the cases it holds.

Each shard writes its cases to `test_results/gtest/<shard>.xml` in the build tree.
`tools/testing/test_budget.py --cases <folder>` reads those files, so the per-case limits apply to
a case in a shard. `GTEST_SHUFFLE` and `GTEST_RANDOM_SEED` of the calling environment reach every
shard.

`sp test --label small` runs one size; `sp test --package` also builds `tests/package/`, an
out-of-tree consumer of the installed package.
