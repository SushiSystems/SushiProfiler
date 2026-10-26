# tests/capture/collector

- `test_collector.cpp`: the collector opens once and refuses a second open; a new thread of a closed
  collector gets no ring; a drain returns what the thread pushed; a ring of 64 that took 70 events
  reports 6 drops; `open` discards events from before; a thread registered while closed keeps its
  ring of the size it asked; a thread's name reaches the drain; calibration points advance; four
  threads get four rings; an exited thread is drained once with `retired` set and then is gone. Each
  test opens the collector in its fixture and closes and drains it in teardown. `pause` keeps
  the rings' events and the session; `resume` reopens the gate, and does nothing while closed. A
  zone recorded by `second_image.cpp`, built as the shared library `sp_second_image`, reaches the
  same collector and drain as a zone of the test executable, on the calling thread and on a thread
  that only runs the second image; a closed collector records nothing from it.
- `test_collector_settings.cpp`: rings of 65,536 events and a filter that keeps every site by
  default.
- `test_drain_batch.cpp`: an empty drain holds no thread and no drop.
