# Changelog

## Unreleased

- 2026-10-26 — window: Added `Window`, which places each period of the followed timeline in a PyTorch-style schedule and opens the collector only for warm-up and active periods (`modules/session/window/`).
- 2026-10-26 — config: Added the profiler configuration and its strict reader, which keeps a default for every key left out (`modules/session/config/`, `docs/reference/CONFIG_FORMAT.md`).
- 2026-10-26 — json: Added `JsonFieldReader`, the strict field reader that names the JSON path of each refusal, moved from the record reader so other file readers share it (`modules/foundation/json/`).
- 2026-10-26 — checks: Added checks read from JSON, which bound a statistic of a subject, and `check`, which turns a record into pass or fail verdicts with reasons (`modules/record/checks/`).
- 2026-10-26 — compare: Added `compare_records`, which compares two records subject by subject and names a change only when the median intervals do not overlap (`modules/record/compare/`).
- 2026-10-26 — analysis: Added `subject_samples`, the samples of every timeline, group, zone, counter, value, span and flow under the names checks and comparisons use (`modules/record/analysis/`).
- 2026-10-26 — record_writers: Added the terminal writer, which prints analysis tables aligned with readable units, and the record file writer (`modules/record/record_writers/`).
- 2026-10-26 — analysis: Added the span analyses device_idle, top_spans and short_spans (`modules/record/analysis/`).
- 2026-10-26 — analysis: Added named analyses over a record, with a catalog and a runner: breakdown, distribution, budget, counters, flows and overhead (`modules/record/analysis/`).
- 2026-10-26 — statistics: Added type-7 percentiles, the coefficient of variation, jitter, a seeded bootstrap interval of the median, MSER-5 and a log2 histogram (`modules/record/statistics/`).
- 2026-10-26 — record_json: Added the record file format, written byte for byte the same and read strictly, with each refusal naming its JSON path (`modules/record/record_json/`, `docs/reference/RECORD_FORMAT.md`).
- 2026-10-26 — record: Added the run record: periods with zone trees, span columns with attributes, and sample series, keyed by the collector's ids (`modules/record/record/`).
- 2026-10-26 — periods: Added `PeriodObserver`, called around each mark, and `pause` and `resume` on the collector, so a capture window opens and closes the gate at period boundaries (`modules/capture/periods/`).
- 2026-10-26 — json: Added a JSON reader and writer that keep 64-bit integers exact and name the line and column of an error (`modules/foundation/json/`).
- 2026-10-26 — docs: Closed the capture layer, design phase P1, and described its record path (`docs/architecture/ARCHITECTURE.md`).
- 2026-10-26 — zones: Measured the hot path, 1.3 ns with the gate closed and 13.3 ns per recorded zone on a Ryzen 5 7600X, and proved a recording thread never allocates after its sites' first use (`tests/capture/zones/`).
- 2026-10-26 — cli: Added `sp build --benchmarks` and `sp bench`, which build and run the benchmark programs (`cli/sushiprofiler/`).
- 2026-10-26 — build: Added `PRIVATE_DEPENDS` to `sushiprofiler_add_module`, for a module whose `source/` files alone include a dependency (`cmake/SushiProfilerModule.cmake`).
- 2026-10-26 — periods: Added named period timelines, whose marks number periods from 0 whether or not the collector records and carry the index, a budget and tags (`modules/capture/periods/`).
- 2026-10-26 — lock_wait: Added the contended lock guard, which records the time a lock waits only when another thread holds it (`modules/capture/lock_wait/`).
- 2026-10-26 — zones: Added the recording macros and the scoped zone, which writes one 24-byte event per zone, with counters, values, instants, flows, lanes and caller-timed spans, each compiled out above a level (`modules/capture/zones/`).
- 2026-10-26 — collector: Added the collector, which opens and closes recording, gives each thread its ring and hands every ring to one consumer, draining an exited thread's ring once before freeing it (`modules/capture/collector/`).
- 2026-10-26 — build: Allowed a module to depend on another of its tier along an edge `SUSHIPROFILER_SAME_TIER_EDGES` lists (`cmake/SushiProfilerTiers.cmake`).
- 2026-10-26 — rings: Added each thread's lock-free event ring, which drops and counts an event when full instead of blocking (`modules/capture/rings/`).
- 2026-10-26 — umbrella: Moved the umbrella module to the top tier, so it includes every module without an upward include (`modules/tools/umbrella/`).
- 2026-10-26 — sites: Added the site registry, which gives each recording site one id on first use and turns sites on and off by level and group at run time, and the name table (`modules/capture/sites/`).
- 2026-10-26 — events: Added the 24-byte trace event and its 14 kinds, with 64-bit times and value bits kept exactly (`modules/foundation/events/`).
- 2026-10-26 — clock: Added the trace clock, which reads the time-stamp counter under MSVC as under clang and calibrates it to steady time (`modules/foundation/clock/`).
- 2026-10-26 — repo: Created the repository skeleton with the sp CLI, the tier table, the version and umbrella modules and the documentation tree (`cli/`, `tools/`, `docs/`).
- 2026-10-26 — design: Opened the PROFILER topic, the design the owner approved for the stack's one profiler (`docs/design/PROFILER/`).
- 2026-10-26 — build: Stopped the post-build copy failing for a test program that loads no DLL, and kept extraction timestamps on the GoogleTest download (`cmake/`).
