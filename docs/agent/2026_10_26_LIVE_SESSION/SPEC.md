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
| Read the newest closed period every frame (RW-14) | `Session::live()` returns a `LiveView`. At each period close the consumer publishes an immutable `ClosedPeriod` (index, timeline, begin, end, budget, zone trees per thread and lane, zone runs capped by `live.max_runs` and `live.min_run_ns`, the span table, the sample series, late and dropped counts) as `std::shared_ptr<const ClosedPeriod>`. `LiveView::latest()` takes a short library lock (`std::atomic_load` on a `std::shared_ptr`, a refcount copy); `LiveView::history(n)` copies up to `live.history` pointers from a ring guarded by a mutex the consumer holds only for the pointer swap, never while folding |
| Names without calling the collector | Each `ClosedPeriod` holds a `std::shared_ptr<const NameSnapshot>` of the names, sites with groups, tracks and threads with names. The tables are append-only, so a snapshot is rebuilt only when one of them grew since the last; periods share it otherwise |
| Group self time, unaccounted time, per-thread totals | The arithmetic of the `breakdown` analysis moves into a free function over one period's record, which both `breakdown` and the editor call |
| Retention | Configuration key `record.keep_periods`: absent keeps every period, `0` keeps none, `N` keeps the last N. `Session::stop()` returns what was kept |
| Writers on and off inside a session | Writers are still named at start, each may start disabled (`"enabled": false`), and `Session::set_writer_enabled(name, bool)` turns one on or off at a period boundary. A writer turned on receives periods from the next close |
| Pause around a module unload | `Session::pause()` and `resume()` close the zone gate through a second reason beside the window's (`Collector::hold` and `release`), so the window's next resume cannot undo a pause; period marks are still written, so periods keep their boundaries, and a period that overlaps a pause carries the fact `paused` = `true`. A second `start` after `stop` in one process is stated and tested (RW-20) |
| Every period (RW-19) | `window.active` takes `"all"`, recording every period after warm-up |
| A per-period fact from a source (RW-7) | `SpanSink::set_fact(name, value)` sets a fact on the period it fills |
| Sources whose work arrives frames late | `Source::max_delay_ns()` (default 0) states how late a source's work can arrive. A period closes once the drain time passes its end plus the larger of `consumer.linger_ms` and every started source's delay, so a GPU source that reads back three frames late is collected whole. The live view lags by that wait, and nothing else does |
| Lag of the live view | Measured at the first build: a benchmark marks a 60 Hz timeline and reports the time from a period's end to its `latest()` publication, for linger 0 and for a 50 ms source delay. The consumer's slowest poll while a `LiveView` is held is bounded by `live.max_poll_ms` (default 4) |
| A lock passed to a condition variable | `SP_LOCK_WAIT_NAMED(lock, name, mutex)` declares `std::unique_lock<Mutex> lock`, tries first and opens a lock-wait zone only when the try fails, so the lock passes to `std::condition_variable::wait` |
| Naming a thread | `SP_THREAD_NAME(name)` in `zones/macros.hpp`, forwarding to `Collector::set_thread_name` |
| A source that pays for device timing only while a session wants it | `Collector::sources_wanted()`, one relaxed load, true from the session's `begin` of its sources until their `end` |
| Counters and values named at run time (RW-13) | `SP_COUNTER_NAMED_AT`, `SP_VALUE_NAMED_AT` and their level-defaulted forms take a name built at run time. `SP_COUNTER_AT` cannot: its name sits in a `static constexpr Site`. `SP_ZONE_NAMED_AT` does not intern either; it takes an interned `name_id`. Each distinct run-time name becomes one site, registered on first use, at most 4096 names |

## Version

`Source` gains a virtual with a default, the configuration gains keys, and `PeriodRecord` gains
nothing it did not have; the release is v0.3.0 because `Session`'s surface grows by several calls.
