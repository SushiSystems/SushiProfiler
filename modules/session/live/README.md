# live

What a live tool reads while a session runs, in the namespace `SushiProfiler`. The module depends
on `record`. A closed period is built once, on the consumer thread, and never changes; a reader
holds it for as long as it likes. How a frame loop uses it is `docs/guides/LIVE_VIEW.md`.

- `SushiProfiler/live/closed_period.hpp`: `ClosedPeriod` holds one period of a run: its `run`
  number from 1, its `sequence` from 0 within the run, `origin_steady_ns` (add it to a record
  time for the steady clock), the `PeriodRecord` with its zone runs capped, `runs_omitted`, the
  run's cumulative `LiveCounts` of late, dropped, span-dropped and outside events as of the
  close, and a `NameSnapshot`. `make_closed_period` copies a period; it drops runs shorter than
  `RunCap::min_run_ns`, keeps the `max_runs` longest of the rest in their original order, and
  copies every other field by hand, so a new `PeriodRecord` field is added there.
- `SushiProfiler/live/name_snapshot.hpp`: `NameSnapshot` copies the names, sites, tracks and
  thread names a period's ids refer to, so a reader never asks the collector. `NameSnapshotCache`
  returns the last snapshot while no table grew and no thread name changed, so periods share one.
  `NameSnapshot::sites()` is the site table `break_down_period` takes.
- `SushiProfiler/live/live_channel.hpp`: `LiveChannel` holds the newest period and a ring of the
  last `history_capacity`. `publish` is the consumer's; `latest()` is `std::atomic_load` on a
  shared pointer, which takes a short library lock; `history(count)` copies up to `count`
  pointers, oldest first, under a mutex that only moves pointers. `begin_run()` empties the
  history and the latest pointer and numbers the run. `watched()` is true while a `LiveView`
  exists.
- `SushiProfiler/live/live_view.hpp`: `LiveView`, the copyable reader handle. Each copy counts as
  a viewer. It shares the channel, so it stays valid after the session stops and restarts.
- `SushiProfiler/live/period_publisher.hpp`: `PeriodPublisher` is the one object the assembler
  calls at a close. It owns the name snapshot cache, the sequence and the run cap, and does
  nothing while the channel is not watched.

The tests are in `tests/session/live/`.
