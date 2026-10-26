# session

The profiling session, in the namespace `SushiProfiler`: the entry point a host uses. The module
depends on `analysis`, `config`, `consumer`, `periods`, `record`, `record_writers`, `registry`,
`sources`, `stream_writers` and `window`, and privately on `collector`, `environment` and
`sites`.

- `SushiProfiler/session/session.hpp`: `Session`.
  - `Session::start(config, registry)` returns a `SessionStart`: the session, or why it did not
    start.
  - `timeline(name)` returns the session's `PeriodTimeline` of that name, making it on first use.
  - `stop()` returns a `SessionStop`: the `RunRecord` and the problems met outside it.
  - `report(record)` runs the configuration's analyses and record writers on a record and returns
    a `SessionReport` of tables and problems.

```cpp
SushiProfiler::Registry registry;
SushiProfiler::register_builtin_writers(registry, std::cout);
SushiProfiler::SessionStart started = SushiProfiler::Session::start(config, registry);
SushiProfiler::PeriodTimeline& step = started.session->timeline("train.step");
for (int iteration = 0; iteration < iterations; ++iteration)
{
    step.mark();
    SP_ZONE("train.iteration");
}
step.mark();
const SushiProfiler::SessionStop stopped = started.session->stop();
const SushiProfiler::SessionReport report = started.session->report(stopped.record);
```

Source: not compiled

## Start

One session runs per process; a second `start` is refused with `a session is already running: `
and the first session's timeline. Before anything is recorded, `start` checks every name: an
unknown analysis is refused with the known ones, and every writer and source is made from the
registry, so an unknown name or bad settings refuse the start. It then makes the output folder,
gathers the environment and build facts from the registry's contributors, adds `config.timeline`,
`config.level` and `config.window` to the environment, and measures the cost of one clock read.

With `enabled` false that is all: the session records nothing and `stop` returns the facts. Else
`start` discards what the rings held, opens the collector with the ring size, level and groups of
the configuration, begins each source and stream writer, and starts the consumer. With a
timeline, the collector opens paused and a `Window` over the configured schedule follows that
timeline, so only its warm-up and active periods are recorded; without one the run is one
period.

## Stop

`stop` removes the window, closes the collector, stops the consumer, which drains a last time and
finishes the record, ends the sources in reverse order and ends the stream writers. A writer that
failed is named in the problems. A second `stop` returns an empty record and says the session was
already stopped. A session dropped without `stop` stops itself and drops its record.

Record writers do not run inside the session: `report` runs them, after the analyses the
configuration names.

The tests are in `tests/session/session/`.
