# tests/session/consumer

- `test_consumer.cpp`: the poll speeds up over half full, holds between a quarter and a half and
  doubles up to 1 s when quiet; a consumer over the real collector records three zones of the
  test thread into the run's one period, counts at least its final drain, and a second `stop`
  does nothing.
- `benchmark_consumer.cpp`: the program `sp_bench_consumer` lets a consumer idle for 2 s, then
  prints its drains per second over the next 8 s as `drainer_idle_wakeups`, the design's
  idle-drainer budget.
- `test_period_assembler.cpp`: drained batches in steady ticks, by hand.
  - A run without a timeline is one period from 0 to the last drain, with the names and sites
    copied.
  - With one warm-up and two active periods, three periods are kept, the first marked warm-up,
    and zones before the first mark or after the last end count as outside.
  - A linger of 100 ns keeps the third period open across a drain; a zone that begins in an
    already closed period is late on its thread.
  - A period without its end mark is not kept, and its events count as outside.
  - A budget sets the period its mark opened, and a tag joins the period it falls in.
  - A source's span joins each period, and spans on an unknown track count as dropped.
  - A source gets each period's steady span, interns a name and adds a track while collecting,
    and work is placed by its start time, work older than the span counting as dropped.
  - An active period keeps its zone runs with their depth; a warm-up period keeps none.
  - Stream writers get each period in order with its zone runs; a failing writer is stopped and
    named.
  - Lanes split zone trees; a lane left open counts at stop and an end of a lane not open is
    unmatched.
  - Threads stay in index order with their names, and dropped events add up across drains.

- `assembly_rig.hpp`: the collaborators of an assembler for a test: a window that records every
  period until given a schedule, a pause log, a live channel and its publisher, and a writer set.
- `test_period_retention.cpp`: absent keeps all; 0 clears; the list is trimmed when it reaches
  twice `keep` and exactly at the end; the newest periods stay in order.
- `test_pause_log.cpp`: overlap across the start and the end of a pause, a boundary touch that
  is none, an open pause, an idempotent `begin`, forgetting, `ends_after`, the cap, and one
  writer against two readers.
- `test_stream_writer_set.cpp`: a disabled writer is not begun until enabled; a request applies
  only in `apply_requests`; turning off keeps the output open until the end; a failed begin or
  write stops the writer and is named; `end_all` ends each begun writer once; an unknown name is
  refused; a thread toggling 1 000 times never tears a writer's begin, write and end order.
- `test_period_assembler.cpp` also covers retention (`keep_periods`, trimmed periods still count
  late events, an event older than the tracked spans counts as outside, tables grow without
  being recopied), a source's fact reaching the period, pauses (the `paused` fact, a paused
  period keeps its marks and has no zones, its sources are still collected, a lane open across a
  pause is reset, old pause intervals are forgotten once every live thread wrote later and a
  retired thread does not hold that back), writer switches at a close, and publication (each
  period in order, nothing unwatched, a run cap that leaves the record whole, a period that
  outlives the assembler, publication after the writers and the record, `keep_periods` 0).
- `test_consumer.cpp` also covers the poll ceiling and a watched consumer's drain rate, and that
  `wake()` ends a long sleep.
