# Changelog

## Unreleased

- 2026-11-10 — stream_writers: Added `ChromeTraceEvents`, which builds the Chrome trace events apart from the file `ChromeTraceWriter` writes (RW-32, `a6ece9b`).
- 2026-11-10 — session: Gave Session a poll clock for the consumer and the pause log the collector's clock, so its tests need no sleep (RW-31, `e855927`).
- 2026-11-10 — session: Inject a clock into Consumer so the live poll tests need no wall clock (RW-30, `53e95f6`).
- 2026-11-10 — continuous_integration: Cleared the known findings of the shared checkers (RW-25, `5289974`).
- 2026-11-10 — lock_wait: Added `SP_LOCK_WAIT_IN`, `SP_LOCK_WAIT_IN_AT`, `SP_LOCK_WAIT_NAMED_IN` and `SP_LOCK_WAIT_NAMED_IN_AT`, which record the wait in a `Collector` the caller passes; the existing macros keep the process-wide one.
- 2026-11-09 — tests: Fixed the test defects of the review that sit in one test: the collector reopen test owns its collector, the no-op build-type test is gone, the package test is medium.
- 2026-11-09 — session: Added the in-memory adapters `MemorySource`, `MemoryStreamWriter`, `MemoryRecordWriter` and `MemoryContributor` beside their ports, and moved the session tests onto them.
- 2026-11-09 — collector: Let a caller build a `Collector` on a `TickSource` and pass it to zones, records, timelines, the window and `Session::start`; the macros keep the process-wide one (`collector.hpp`, `tick_source.hpp`).
- 2026-11-09 — tests: Derived every tolerance of the record tests through `tests/support/tolerance.hpp` in place of a typed epsilon.
- 2026-11-06 — cli: Gave the project service a `Workspace` and moved the CLI tests onto sushicore's in-memory runner and console; the CLI asks for sushicore 0.10.12.
- 2026-11-06 — tools: Deleted the leftover `check_test_additions.py` and its test, which the shared tools no longer carry, and recopied the shared tools (RW-25).
- 2026-11-06 — licence: Relicensed SushiProfiler under the Apache License 2.0 and removed `COMMERCIAL.md`; every source file's licence block follows (`LICENSE`, `tools/settings.py`).
- 2026-11-05 — tools: Took the shared checkers of 2026-11-05 with a baseline of the findings they meet, which RW-25 clears.
- 2026-10-31 — cli: Ran `sp test` through the shared test options and `run_tests` of sushicore, so it runs small and medium by default and prints the budget line (RW-24, `4cb86b8`).
- 2026-10-30 — continuous_integration: Turn the red CI green: checkers, package test, and the sanitizer allocation counter (RW-23, `799daf1`).
- 2026-10-29 — build: Let a Debug tree link an imported package built as `None`, such as Ubuntu's GTest (`cmake/`).
## v0.4.1 — 2026-10-29

- 2026-10-29 — version: Set `version.hpp` to the version `project()` declares, so the v0.4.0 configure no longer stops on the mismatch (`version.hpp`).
