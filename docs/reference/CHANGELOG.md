# Changelog

## Unreleased

- 2026-10-26 — lock_wait: Added the contended lock guard, which records the time a lock waits only when another thread holds it (`modules/capture/lock_wait/`).
- 2026-10-26 — zones: Added the recording macros and the scoped zone, which writes one 24-byte event per zone, with counters, values, instants, flows, lanes and caller-timed spans, each compiled out above a level (`modules/capture/zones/`).
- 2026-10-26 — collector: Added the collector, which opens and closes recording, gives each thread its ring and hands every ring to one consumer, draining an exited thread's ring once before freeing it (`modules/capture/collector/`).
- 2026-10-26 — build: Allowed a module to depend on another of its tier along an edge `SUSHIPROFILER_SAME_TIER_EDGES` lists (`cmake/SushiProfilerTiers.cmake`).
- 2026-10-26 — rings: Added each thread's lock-free event ring, which drops and counts an event when full instead of blocking (`modules/capture/rings/`).
- 2026-10-26 — umbrella: Moved the umbrella module to the top tier, so it includes every module without an upward include (`modules/tools/umbrella/`).
- 2026-10-26 — sites: Added the site registry, which gives each recording site one id on first use and turns sites on and off by level and group at run time, and the name table (`modules/capture/sites/`).
- 2026-10-26 — events: Added the 24-byte trace event and its 13 kinds, with 64-bit times and value bits kept exactly (`modules/foundation/events/`).
- 2026-10-26 — clock: Added the trace clock, which reads the time-stamp counter under MSVC as under clang and calibrates it to steady time (`modules/foundation/clock/`).
- 2026-10-26 — repo: Created the repository skeleton with the sp CLI, the tier table, the version and umbrella modules and the documentation tree (`cli/`, `tools/`, `docs/`).
- 2026-10-26 — design: Opened the PROFILER topic, the design the owner approved for the stack's one profiler (`docs/design/PROFILER/`).
- 2026-10-26 — build: Stopped the post-build copy failing for a test program that loads no DLL, and kept extraction timestamps on the GoogleTest download (`cmake/`).
