# tests/session/consumer

- `test_consumer.cpp`: the poll speeds up over half full, holds between a quarter and a half and
  doubles up to 1 s when quiet; a consumer over the real collector records three zones of the
  test thread into the run's one period, and a second `stop` does nothing.
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
  - Stream writers get each period in order with its zone runs; a failing writer is stopped and
    named.
  - Lanes split zone trees; a lane left open counts at stop and an end of a lane not open is
    unmatched.
  - Threads stay in index order with their names, and dropped events add up across drains.
