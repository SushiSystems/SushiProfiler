# tests/session/window

- `test_window.cpp`: with one skipped, one waiting, one warm-up and two active periods over two
  cycles, each period's phase up to `Done` is a row of one test, and a warm-up period is recorded
  while a waiting one is not; `repeat` 0 stays active after a thousand cycles; on a real timeline
  with the collector paused (a medium test, since it reaches the process-wide collector), seven
  marks leave the marks of periods 2 to 5 in the ring and the collector paused.
- With `active_all`, `skip`, `wait` and `warmup` apply once and every period after is active,
  however late (rows of one test); recording starts at the end of the lead-in; no warm-up records
  from the first period.
