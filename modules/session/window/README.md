# window

The capture window, in the namespace `SushiProfiler`. It decides which periods of the followed
timeline are recorded and opens the collector's gate only for them, so periods outside the window
cost what an off profiler costs. The module depends on `config` and `periods`, and privately on
`collector`.

- `SushiProfiler/window/window.hpp`: `Window` takes a `WindowSchedule` from the configuration.
  `phase_of(index)` returns the `WindowPhase` of a period: `Skipped` for the first `skip`
  periods, then per cycle `Waiting` for `wait` periods, `Warmup` for `warmup` and `Active` for
  `active`, and `Done` after `repeat` cycles; `repeat` 0 never reaches `Done`. With
  `WindowSchedule::active_all`, the skipped periods are `Skipped`, the next `wait` are `Waiting`,
  the next `warmup` are `Warmup` and every later period is `Active`; there is no `Done`.
  `records(index)` is true for `Warmup` and `Active`.

`Window` is a `PeriodObserver`. The session registers it on the followed timeline with
`PeriodTimeline::observe` and opens the collector paused. Before the mark that opens a recorded
period the window resumes the collector, so that mark is written; after the mark that opens a
period that is not recorded it pauses the collector, so the last recorded period still gets the
mark that ends it. The record therefore holds the marks of the recorded periods and of the one
after them.

The tests are in `tests/session/window/`.
