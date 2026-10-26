# tests/session/config

- `test_profiler_config.cpp`: the design's example reads in full; a key left out keeps its
  default; a missing or other version is refused; unknown keys are refused at the top and inside
  `window`; `level`, `window.active`, `buffers.ring_events` and `buffers.linger_ms` out of range
  are refused; a group that is not a string, writer settings that are not an object and a flag
  that is not a bool are refused.
