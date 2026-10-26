# tests/session/session

- `test_session.cpp`:
  - End to end on two threads: the main thread marks `e2e.step` eleven times and records a zone,
    a counter, a value, an instant and a flow begin each step, and a worker thread records a
    zone and the flow's end. With one warm-up and nine active periods the record holds ten
    periods, the first warm-up, each with both threads' zones and every sample site, and both
    thread names. The report's four tables reach the terminal, the record file reads back equal
    to the record, and the Chrome trace parses. The `flows` table has a `total` and a `step`
    row, the record compares with itself, and a check that the flow's maximum is at most 1 ns
    fails while a check that every step lasts at least 1 ns passes.
  - An unknown analysis and an unknown writer refuse the start, listing the known names.
  - A second session is refused while the first runs, and allowed once it stopped.
  - A disabled session records nothing, states its facts, and a second `stop` says so.
