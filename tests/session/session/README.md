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
  - An unknown analysis and an unknown writer each refuse the start, listing the known names.
  - A second session is refused while the first runs, and allowed once it stopped.
  - A disabled session records nothing, states its facts, and a second `stop` says so, one test
    each; `keep_periods` unset, 0 and 7 are the facts `all`, `0` and `7` (rows of one test).
  - These tests start a session, which takes the process's one running slot, so they are medium
    tests. The ones that move time or wait for a drain run on a collector of their own over a
    `ManualTickSource` and a `ManualPollClock` (the `ManualRun` of `test_session.cpp`): the test
    passes time and lets the consumer drain, and no test sleeps or waits on a deadline.
  - A live view shows each closed period with consecutive sequences and resolvable names, outlives
    the session, and keeps working across a restart with the new run's periods.
  - `keep_periods` N leaves N periods in the stopped record (which ones is the rows of
    `tests/session/consumer/test_period_retention.cpp`); 0 leaves none while the view still
    publishes; the record states it as `config.record.keep_periods`.
  - A writer that starts disabled writes no file until switched on, then only later periods; a
    toggle for an unknown name, a record writer or a stopped session is refused.
  - `pause` and `resume` take effect on a running session and not on a disabled or stopped one
    (the transitions are `test_pause_state.cpp`); a fixture runs six periods paused across the
    second to the fourth and each promise is a test: the indices stay continuous, those periods
    carry `paused`, no zone is recorded during the pause, the zones before are kept and
    recording resumes; the collector's `wants_sources()` is false while paused and outside a run.
  - A source delay of 200 ms holds periods open: no span is lost and each period holds its span,
    handed over three periods late; a delay above 60 000 ms refuses the start naming the source.
  - Every-period windows record all periods after the warm-up; a restart reads a second record
    that holds none of the first; a source's fact survives the record file; control calls from
    one thread against `stop` and `restart` from another return only what the contract names;
    a restart while the session runs is refused; the source fact is checked in the record and, in
    a test of its own, in the record file.
  - The end-to-end run is one fixture; each promise (ten periods, both threads in every period,
    thread names, the four tables, the `flows` rows, the self comparison, the checks, the record
    file and the trace) is its own test.
- `test_pause_state.cpp`: a pause takes effect once and only on a running session; a resume once
  and only on a paused, running one; `clear` forgets a pause.

`test_session.cpp` keeps two sources of its own, which read the collector's wish for sources inside
`begin`, `collect` and `end`; `MemorySource` cannot see the collector.
