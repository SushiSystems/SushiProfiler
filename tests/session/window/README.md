# tests/session/window

- `test_window.cpp`: with one skipped, one waiting, one warm-up and two active periods over two
  cycles, every period's phase is checked up to `Done`; `repeat` 0 stays active after a thousand
  cycles; on a real timeline with the collector paused, seven marks leave the marks of periods 2
  to 5 in the ring and the collector paused.
- With `active_all`, every period after the warm-up is active, however late; `skip`, `wait` and
  `warmup` apply once; no warm-up records from the first period.
