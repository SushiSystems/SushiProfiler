# Contributing to SushiProfiler

Contributions from outside Sushi Systems are not accepted yet. SushiProfiler is source-available
under the PolyForm Noncommercial License 1.0.0; see [`LICENSE`](../LICENSE) and
[`COMMERCIAL.md`](../COMMERCIAL.md).

## Building and testing

```bash
pip install -e ./cli
sp build
sp test --label small --label medium --package
sp check
```

Every build and test goes through `sp`; [`BUILD_AND_TEST.md`](getting_started/BUILD_AND_TEST.md)
lists the commands.

## What a change contains

- A module is a folder under `modules/<tier>/` and one line in `cmake/SushiProfilerTiers.cmake`.
  A change that needs an edit to another module has drawn the boundary in the wrong place.
- A module depends on the C++17 standard library and Threads alone. A new dependency is the
  owner's decision.
- Every public header has its test at the mirrored path under `tests/`, and every test names its
  size in its suite: `Small_`, `Medium_` or `Large_`.
- The module README, the pages under `docs/` that describe the change and a changelog line land
  in the same commit.
- `python tools/check_all.py .` is clean before the change is reported finished.
