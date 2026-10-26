# consumer

The consumer turns what the rings hold into the record's periods while a session runs, in the
namespace `SushiProfiler`. The module depends on `clock`, `collector`, `fold`, `live`, `record`,
`sites`, `sources`, `stream_writers` and `window`.

- `SushiProfiler/consumer/period_assembler.hpp`: `PeriodAssembler` fills a `RunRecord` from
  drained batches. `AssemblySettings` holds values: the followed timeline, the linger, the
  collector's first calibration point, which is record time 0, and `keep_periods`. The
  collaborators come in one `AssemblyParts` of references that outlive the assembler: the
  `Window`, the `PauseLog`, the `PeriodPublisher` and the `StreamWriterSet`. `add(batch)` takes
  one drain; `finish()` places what waits, closes what ended and trims the record to
  `keep_periods`; `end_writers()` ends the stream writers; `problems()` lists what went wrong
  outside the record.
- `SushiProfiler/consumer/period_retention.hpp`: `trim_periods(periods, keep, final_trim)` keeps
  the newest `keep` periods. While a run goes it trims once the list holds `2 * keep`, so erasing
  the front costs one move per kept period; at the end it trims to exactly `keep`; 0 keeps none
  and an absent value keeps all.
- `SushiProfiler/consumer/pause_log.hpp`: `PauseLog` holds the intervals a session was paused on
  the steady clock, behind one mutex. The session calls `begin` before it holds the gate and
  `end` before it releases it. `overlaps` says whether a period crossed a pause, `ends_after`
  gives the pause ends the assembler has not read, and `forget_before` drops old intervals; it
  keeps at most 4096. A period that overlaps a pause carries the fact `K_PAUSED_FACT`, `paused`,
  with the value `true`; its zones are partial, because the gate was closed, and its sources are
  still collected.
- `SushiProfiler/consumer/stream_writer_set.hpp`: `StreamWriterSet` holds the stream writers and
  which receive periods. Any thread may `request` a writer on or off; the consumer applies the
  requests at the start of a period's close, so a writer changes state between periods. A writer
  that starts disabled is begun, and opens its output, the first time it is enabled. A writer
  that fails is stopped and named in the problems; `end_all` ends each begun writer once.
- `SushiProfiler/consumer/consumer.hpp`: `Consumer` drains a `Collector` into an assembler on
  its own thread. `stop()` drains once more, finishes the assembler and joins; the session closes
  the collector first, so that drain is the last. `next_poll` sets the wait between drains: 1 ms
  while a ring is over half full, the same wait while the fullest ring is between a quarter and
  a half, and double the wait, up to 1 s, while every ring is under a quarter. While a
  `LiveView` of the channel exists the ceiling is `live.max_poll_ms`, 4 ms by default, and
  `wake()` ends the current wait so a new viewer is served at once. `drains()` counts the drains
  done, the final one included, so an unwatched idle consumer settles at one a second.

## How events become periods

1. Each drain converts times to the record's clock with a `RecordClock` calibrated from the
   first point to the drain's point.
2. Marks and budgets of the followed timeline are read first, from every thread, in time order.
   A mark ends the open period and, when the window records its index, opens the next, a
   warm-up period when the window says so. A budget sets the open period's budget.
3. Every other event gets its lane from its thread's `LaneTracker` and waits. When the window
   opens the gate again after a gap, or a pause ends, each thread's lanes are reset at its first
   event written after that time. Reopen times no live thread can still meet are dropped at every
   close, and a retired thread does not hold them back.
4. An event joins a period once the drain time less the linger passes its begin. It joins the
   open period its begin falls in, as a tag when it is one of the followed timeline's tags; it
   counts as late on its thread when the period that holds its begin already closed, and as
   outside the periods otherwise.
5. A period closes once the drain time less the linger passes its end. Its events are folded
   with `fold_period`, each source's `collect` receives the period's steady span and adds its spans
   through a `SpanSink` whose refusals and late drops count as dropped spans, and the facts a
   source sets join the period. A period that crossed a pause gets `paused`. The stream writers
   write it, the record appends it, and the `PeriodPublisher` publishes it to a watching view;
   only then is the record trimmed to `keep_periods`, so `keep_periods` 0 still publishes. A
   writer that fails is stopped and named in `problems()`. The assembler remembers the closed
   spans, coalesced and at most 4096, to count a late event; an event older than those counts as
   outside.
6. At `finish`, a period of the followed timeline without its end mark is not kept and its events
   count as outside; lanes still open count as open at stop, and lane ends with no lane open as
   unmatched on their thread.

With no timeline the run is one period, index 0, from record time 0 to the last drain, named by
`run_timeline`. Thread records stay in index order with their names and dropped events; the
record's names and sites are appended from the session's tables, only the entries added since the
last period, before each period is written. `tests/session/consumer/assembly_rig.hpp` owns the
collaborators of an assembler for a test.

The tests are in `tests/session/consumer/`.
