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
  - A live view shows each closed period with consecutive sequences and resolvable names, outlives
    the session, and keeps working across a restart with the new run's periods.
  - `keep_periods` leaves the newest N periods in the stopped record; 0 leaves none while the view
    still publishes; the record states it as `config.record.keep_periods`.
  - A writer that starts disabled writes no file until switched on, then only later periods; a
    toggle for an unknown name, a record writer or a stopped session is refused.
  - `pause` and `resume` report whether they took effect; the periods that crossed a pause carry
    `paused`, hold no zone recorded during it, and recording resumes with continuous indices;
    `Collector::sources_wanted()` is false while paused and outside a run.
  - A source delay of 200 ms holds periods open so spans handed over three periods late are all
    placed; a delay above 60 000 ms refuses the start naming the source.
  - Every-period windows record all periods after the warm-up; a restart reads a second record
    that holds none of the first; a source's fact survives the record file; control calls from
    one thread against `stop` and `restart` from another return only what the contract names.
