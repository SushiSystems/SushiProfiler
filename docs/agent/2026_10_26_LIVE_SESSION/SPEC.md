# SushiProfiler v0.3.0: a session a live tool reads

**Status:** Open — written 2026-10-26; decided by the orchestrator on the owner's delegation, nothing built.

**Implements:** PROFILER#P2c

SushiEngine's editor shows a profiler window that redraws every frame from the newest closed
period, keeps a 300-frame history, and starts and stops captures inside one long session. v0.2.0
returns a record only at `stop`, grows it without bound, fixes its writers at start and closes a
period at one linger for every source. SushiEngine's session sent the editor's contract on
2026-10-26; this release answers it. v0.2.0 ships first, unchanged, because the inference path
needs none of this.

## Decisions

| Need | Decision |
| --- | --- |
| Read the newest closed period every frame (RW-14) | `Session::live()` returns a `LiveView`. At each period close the consumer publishes an immutable `ClosedPeriod` (index, timeline, begin, end, budget, zone trees per thread and lane, zone runs capped by `live.max_runs` and `live.min_run_ns`, the span table, the sample series, late and dropped counts) as `std::shared_ptr<const ClosedPeriod>`. `LiveView::latest()` is one atomic load; `LiveView::history(n)` copies up to `live.history` pointers from a ring guarded by a mutex the consumer holds only for the pointer swap, never while folding |
| Names without calling the collector | Each `ClosedPeriod` holds a `std::shared_ptr<const NameSnapshot>` of the names, sites with groups, tracks and threads with names. The tables are append-only, so a snapshot is rebuilt only when one of them grew since the last; periods share it otherwise |
| Group self time, unaccounted time, per-thread totals | The arithmetic of the `breakdown` analysis moves into a free function over one period's record, which both `breakdown` and the editor call |
| Retention | Configuration key `record.keep_periods`: absent keeps every period, `0` keeps none, `N` keeps the last N. `Session::stop()` returns what was kept |
| Writers on and off inside a session | Writers are still named at start, each may start disabled (`"enabled": false`), and `Session::set_writer_enabled(name, bool)` turns one on or off at a period boundary. A writer turned on receives periods from the next close |
| Pause around a module unload | `Session::pause()` and `resume()` over the collector's existing pause; a closed gate during pause, periods during it are marked paused. A second `start` after `stop` in one process is stated and tested (RW-20) |
| Every period (RW-19) | `window.active` takes `"all"`, recording every period after warm-up |
| A per-period fact from a source (RW-7) | `SpanSink::set_fact(name, value)` sets a fact on the period it fills |
| Sources whose work arrives frames late | `Source::max_delay_ns()` (default 0) states how late a source's work can arrive. A period closes once the drain time passes its end plus the larger of `consumer.linger_ms` and every started source's delay, so a GPU source that reads back three frames late is collected whole. The live view lags by that wait, and nothing else does |
| Lag of the live view | Measured at the first build: a benchmark marks a 60 Hz timeline and reports the time from a period's end to its `latest()` publication, for linger 0 and for a 50 ms source delay. The consumer's slowest poll while a `LiveView` is held is bounded by `live.max_poll_ms` (default 4) |

## Version

`Source` gains a virtual with a default, the configuration gains keys, and `PeriodRecord` gains
nothing it did not have; the release is v0.3.0 because `Session`'s surface grows by several calls.
