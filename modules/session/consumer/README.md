# consumer

The consumer turns what the rings hold into the record's periods while a session runs, in the
namespace `SushiProfiler`. The module depends on `clock`, `collector`, `fold`, `record`, `sites`,
`sources`, `stream_writers` and `window`.

- `SushiProfiler/consumer/period_assembler.hpp`: `PeriodAssembler` fills a `RunRecord` from
  drained batches. `AssemblySettings` names the followed timeline, the `Window` over it, the
  linger and the collector's first calibration point, which is record time 0. `add(batch)` takes
  one drain; `finish()` places what waits and closes what ended; `problems()` lists what went
  wrong outside the record.
- `SushiProfiler/consumer/consumer.hpp`: `Consumer` drains a `Collector` into an assembler on
  its own thread. `stop()` drains once more, finishes the assembler and joins; the session closes
  the collector first, so that drain is the last. `next_poll` sets the wait between drains: 1 ms
  while a ring is over half full, the same wait while the fullest ring is between a quarter and
  a half, and double the wait, up to 1 s, while every ring is under a quarter.

## How events become periods

1. Each drain converts times to the record's clock with a `RecordClock` calibrated from the
   first point to the drain's point.
2. Marks and budgets of the followed timeline are read first, from every thread, in time order.
   A mark ends the open period and, when the window records its index, opens the next, a
   warm-up period when the window says so. A budget sets the open period's budget.
3. Every other event gets its lane from its thread's `LaneTracker` and waits. When the window
   opens the gate again after a gap, each thread's lanes are reset at its first event written
   after that time.
4. An event joins a period once the drain time less the linger passes its begin. It joins the
   open period its begin falls in, as a tag when it is one of the followed timeline's tags; it
   counts as late on its thread when the period that holds its begin already closed, and as
   outside the periods otherwise.
5. A period closes once the drain time less the linger passes its end. Its events are folded
   with `fold_period`, each source's `collect` adds its spans through a `SpanSink` whose refusals
   count as dropped spans, the stream writers write it, and the record appends it. A writer that
   fails is stopped and named in `problems()`.
6. At `finish`, a period of the followed timeline without its end mark is not kept and its events
   count as outside; lanes still open count as open at stop, and lane ends with no lane open as
   unmatched on their thread.

With no timeline the run is one period, index 0, from record time 0 to the last drain, named by
`run_timeline`. Thread records stay in index order with their names and dropped events; the
record's names and sites are copied from the session's tables before each period is written.

The tests are in `tests/session/consumer/`.
