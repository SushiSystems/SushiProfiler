# tests/capture/collector

- `test_collector.cpp`: the `Small_` tests build their own `Collector` on a `ManualTickSource` that
  starts at 100 ns, so no process-wide state and no real clock is read. A first `open` opens the gate
  and a second one is refused; a drain returns what the thread pushed; `open` discards events from
  before; a thread's name reaches the drain; the calibration points are the manual clock's reading
  at `open` and at each drain. `pause` closes the gate and, as a second test, keeps the rings'
  events and the session; `resume` reopens the gate and does nothing while closed. `hold` closes the
  zone gate and, as a second test, keeps marking; `release` keeps a paused gate closed, `resume`
  keeps a held gate closed, and `release` after both opens it; `close` clears a hold; `hold` does
  nothing while closed. The `Medium_` tests use the process-wide collector and real threads: a new
  thread of a closed collector gets no ring; a ring of 64 that took 70 events reports 6 drops; a
  thread registered while closed keeps its ring of the size it asked; four threads get four rings;
  an exited thread is drained once with `retired` set and then is gone. A zone recorded by
  `second_image.cpp`, built as the shared library `sp_second_image`, reaches the same collector and
  drain as a zone of the test executable, on the calling thread and on a thread that only runs the
  second image; a closed collector records nothing from it.
- `test_collector_settings.cpp`: rings of 65,536 events and a filter that keeps every site by
  default.
- `test_drain_batch.cpp`: an empty drain holds no thread and no drop.
