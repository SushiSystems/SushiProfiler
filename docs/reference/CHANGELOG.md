# Changelog

## Unreleased

## v0.3.0 — 2026-10-27

- 2026-10-26 — deps: Moved the sushicore pin to 0.9.6 (`cli/pyproject.toml`).
- 2026-10-26 — cli: Added `sp build --build-dir` and `--lean`, the contract of a consumer's package build (`cli.py`, `project.py`).
- 2026-10-26 — cli: Registered the shared `package` command from sushicore (`cli.py`).
- 2026-10-26 — cli: Made `sp` honour `SUSHISTACK_CXX` over its configured compiler (`config.py`).
- 2026-10-26 — deps: Added `cli/sushistack.deps.toml`, which names the CLI `sp` in `[module] cli`.
- 2026-10-26 — record: Added `K_GRAPH` and `K_ORIGIN` to `DeviceSpanKeys` (`modules/record/record/`).
- 2026-10-26 — analysis: Changed `roofline` to group by graph and origin, with two new columns after `period` that move every later column by two (`modules/record/analysis/`).
- 2026-10-26 — session: Added live, pause, resume, set_writer_enabled and restart to Session, with source delays, retention and the paused fact (`modules/session/session/`).
- 2026-10-26 — consumer: Added retention, `PauseLog`, `StreamWriterSet` and a poll ceiling; `PeriodAssembler` takes `AssemblyParts`, which breaks programs that build one (`modules/session/consumer/`).
- 2026-10-26 — live: Added `ClosedPeriod`, `LiveChannel`, `LiveView` and `PeriodPublisher`, the immutable periods a tool reads while a session runs (`modules/session/live/`).
- 2026-10-26 — docs: Added the live view guide and corrected the linger key to `buffers.linger_ms` (`docs/guides/LIVE_VIEW.md`, `docs/guides/CLIENT_LIBRARY.md`).
- 2026-10-26 — sources: Added `Source::max_delay_ns` and `SpanSink::set_fact` (`modules/session/sources/`).
- 2026-10-26 — window: Added the every-period schedule, `window.active` `"all"` (`modules/session/window/`).
- 2026-10-26 — config: Added window.active "all", record.keep_periods, the live keys and a writer's enabled flag (`modules/session/config/`, `docs/reference/CONFIG_FORMAT.md`).
- 2026-10-26 — analysis: Added `break_down_period`, one period's self time per group, uncovered time and thread totals; `breakdown` calls it (`modules/record/analysis/`).
- 2026-10-26 — lock_wait: Added `SP_LOCK_WAIT_NAMED`, which declares a `std::unique_lock` a condition variable can wait on (`modules/capture/lock_wait/`).
- 2026-10-26 — zones: Added `SP_COUNTER_NAMED`, `SP_VALUE_NAMED` and `SP_THREAD_NAME` (`modules/capture/zones/`, `modules/capture/sites/`).
- 2026-10-26 — periods: Added `PeriodTimeline::reset`, and marks are written while the zone gate is held (`modules/capture/periods/`).
- 2026-10-26 — collector: Added `hold` and `release`, a second reason to close the zone gate that leaves period marks written, and `sources_wanted` (`modules/capture/collector/`).
- 2026-10-26 — record: Added `DeviceSpanKeys`, the attribute and ceiling fact keys, per type too, that device sources write and `roofline` reads (`modules/record/record/`).
- 2026-10-26 — analysis: Added `roofline`: per span name, node and type, achieved FLOP/s and bytes/s and the share of the bound the ceiling facts give (`modules/record/analysis/`).
- 2026-10-26 — docs: Added the client library guide: the shared or static choice, `register_profiler_parts` and what a source may rely on (`docs/guides/CLIENT_LIBRARY.md`).
- 2026-10-26 — sources: Changed what `collect` receives to a period span and added late-span counting; every Source implementation must change (`modules/session/sources/`).
- 2026-10-26 — record: Added each active period's zone runs as record format 2, which refuses version 1 files, and `device_idle` names the host zone over a gap (`docs/reference/RECORD_FORMAT.md`).
- 2026-10-26 — library: Built `SushiProfiler::SushiProfiler` shared, so every image of a process reaches one collector; `SushiProfiler::SushiProfilerStatic` keeps a collector per plug-in (`cmake/SushiProfilerLibrary.cmake`).
