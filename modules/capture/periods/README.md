# periods

Named timelines that divide a run into numbered periods, in the namespace `SushiProfiler`. It
depends on `events`, `rings` and `collector`.

`SushiProfiler/periods/period_timeline.hpp` declares `PeriodTimeline`. A timeline is built with a
name, such as `train.step` or `engine.frame`, which it interns, and an optional budget in
nanoseconds.

- `mark()` opens the next period and returns its index, from 0. The index advances whether or not
  the collector records. Each recorded mark writes a `PeriodMark` event carrying the index and the
  timeline, and a `PeriodBudget` event when the period has a budget, so a source that stamps its
  own data with the index agrees with the record even when marks were closed out or a full ring
  dropped one. `mark(budget_ns)` gives this period its own budget; 0 gives it none.
- `current()` returns the open period's index, one relaxed load, or `K_NO_PERIOD` before the first
  mark. Several threads may mark one timeline.
- `tag(key, value)` interns both strings and writes a `PeriodTag` the fold attaches to the open
  period. It takes a lock, so a real-time thread does not tag.

`SushiProfiler/periods/period_observer.hpp` declares `PeriodObserver`, which a timeline calls on
the marking thread around each mark: `before_mark`, then the mark is written if the gate is open,
then `after_mark`. `observe(observer)` installs one, or removes it with null. A capture window
opens the gate in `before_mark` and closes it in `after_mark`, so the last recorded period still
gets its end mark.

Marks and tags have no site; only the gate turns them off. The tests are in
`tests/capture/periods/`.
