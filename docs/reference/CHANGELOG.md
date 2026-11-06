# Changelog

## Unreleased

- 2026-11-06 — cli: Gave the project service a `Workspace` and moved the CLI tests onto sushicore's in-memory runner and console; the CLI asks for sushicore 0.10.12.
- 2026-11-06 — tools: Deleted the leftover `check_test_additions.py` and its test, which the shared tools no longer carry, and recopied the shared tools (RW-25).
- 2026-11-06 — licence: Relicensed SushiProfiler under the Apache License 2.0 and removed `COMMERCIAL.md`; every source file's licence block follows (`LICENSE`, `tools/settings.py`).
- 2026-11-05 — tools: Took the shared checkers of 2026-11-05 with a baseline of the findings they meet, which RW-25 clears.
- 2026-10-31 — cli: Ran `sp test` through the shared test options and `run_tests` of sushicore, so it runs small and medium by default and prints the budget line (RW-24, `4cb86b8`).
- 2026-10-30 — continuous_integration: Turn the red CI green: checkers, package test, and the sanitizer allocation counter (RW-23, `799daf1`).
- 2026-10-29 — build: Let a Debug tree link an imported package built as `None`, such as Ubuntu's GTest (`cmake/`).
## v0.4.1 — 2026-10-29

- 2026-10-29 — version: Set `version.hpp` to the version `project()` declares, so the v0.4.0 configure no longer stops on the mismatch (`version.hpp`).
