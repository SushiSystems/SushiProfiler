# Reading closed periods while a session runs

A tool that redraws every frame, such as an editor's profiler window, reads the newest closed
period of a running `Session` and a short history of the ones before it. This page states how, and
what the view costs.

## Taking a view

```cpp
SushiProfiler::LiveView view = started.session->live();
```

Source: not compiled

`Session::live()` returns a `LiveView` and wakes the consumer. While any copy of a view exists, the
consumer publishes each closed period and waits at most `live.max_poll_ms` (4 ms by default)
between drains; without one it publishes nothing and settles at one wake-up a second. A view
shares the session's channel, so it stays valid after `stop()` and across `restart()`.

## Reading a frame

```cpp
const std::shared_ptr<const SushiProfiler::ClosedPeriod> latest = view.latest();
if (latest != nullptr && latest->sequence != last_drawn)
{
    draw(*latest);
    last_drawn = latest->sequence;
}
```

Source: not compiled

- `latest()` returns the newest `ClosedPeriod`, or null before the first of the run. It takes a
  short library lock (an atomic load of a shared pointer), so it is cheap enough for every frame
  and is not lock-free in the standard's sense.
- `history(count)` copies up to `live.history` (300 by default) pointers, oldest first. The
  channel's mutex is held only while pointers move, never while a period is built.
- A `ClosedPeriod` never changes. Hold the pointer as long as the frame needs it; a period you
  hold survives the ring wrapping, a restart and the end of the session.
- `sequence` counts the periods published in the run from 0, and `run` numbers the run from 1, so
  a restart is visible. Periods closed while no view existed are not published and have no
  sequence.

## What a period holds

A `ClosedPeriod` carries the `PeriodRecord` (index, begin, end, budget, zone trees per thread and
lane, the span table and the sample series), the zone runs capped by `live.max_runs` and
`live.min_run_ns` with `runs_omitted` counting the rest, the run's cumulative late, dropped,
span-dropped and outside event counts as of the close, and a `NameSnapshot`. Resolve every id
through that snapshot: `names->name(id)`, `names->site(id)`, `names->track(id)` and
`names->thread_name(index)`. Periods share one snapshot until a table grows.

`break_down_period(period, names->sites())` gives the period's self time per zone group, the time
no zone covered and each thread's totals, the arithmetic the `breakdown` analysis uses.

A period that crossed a `Session::pause()` carries the fact `paused`; its zones are partial.
Times in the period are nanoseconds since the run began; add `origin_steady_ns` to get the steady
clock.

## Lag

A period is published once the drain time passes its end plus the larger of `buffers.linger_ms`
(100 ms by default) and the largest `Source::max_delay_ns()` of the started sources. A tool that
wants frames as they end sets `buffers.linger_ms` to 0; a device source that reads back late
raises the wait for every period, and the view lags by that wait and by at most `live.max_poll_ms`.
Nothing else adds lag.

## Pausing around a module unload

`Session::pause()` stops recording zones and `Session::resume()` records again; period marks keep
being written, so frames keep their boundaries. `Collector::sources_wanted()` is false while the
session is paused, so a source that pays for device timing can stop paying.
