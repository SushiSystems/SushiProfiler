# fold

The fold turns one period's events into what the record keeps, in the namespace `SushiProfiler`.
The module depends on `clock`, `events` and `record`.

- `SushiProfiler/fold/record_clock.hpp`: `RecordClock` moves event times onto the record's
  clock, nanoseconds since the session's first calibration point. `from_ticks` maps ticks
  through the drain's `Calibration`, or takes them as steady nanoseconds when they are not TSC;
  `from_steady` subtracts the origin. A time before the origin is 0. `to_record(event)` converts
  each field that holds a time: both fields of a zone or instant from ticks, both fields of an
  external zone or instant from steady time, and the first field of every other kind.
- `SushiProfiler/fold/lane_tracker.hpp`: `LaneTracker` follows one thread's lane events in ring
  order. `lane_of(event)` returns the innermost open lane, or 0; a lane begin opens its lane and
  a lane end closes the innermost open lane of that name. A lane end with no such lane open is
  counted by `unmatched()`. `reset()` closes every lane, for when the gate was closed while lanes
  changed; `open_lanes()` is what stays open at stop.
- `SushiProfiler/fold/period_fold.hpp`: `fold_period(threads)` takes each thread's `FoldEvent`s
  (an event on the record's clock and its lane) and returns a `PeriodFold`:
  - one `ZoneTree` per thread and lane, in that order. Zones nest by time: they are sorted by
    begin, then longer first, then later in the ring first, so of two runs with equal times the
    one written last, the outer, is the parent. Runs of one site and name under one parent node
    merge into one node; `self_ns` is a run's time outside its direct children. Parents come
    before their children.
  - one `SampleSeries` per site and thread, in that order: counters and values with their
    values, instants with times alone, flow points with item and place.
  - every zone run as a `ZoneRun` (defined in `record`) with its depth, for writers that draw a
    timeline; the consumer keeps the runs of an active period in the record.
  - the unmatched zones per thread: a zone that ends after the zone open around it becomes that
    zone's sibling, and a zone that ends before it begins is dropped; both are counted.

Period and lane events are not folded; the consumer reads them to place events in periods and
lanes. A zone belongs to the period its begin falls in, so a zone that begins in one period and
ends in the next is folded whole into the first, and a child that begins in the next period is a
root there. Folding the same events gives the same result.

The tests are in `tests/session/fold/`.
