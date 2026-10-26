# tests/session/config

- `test_profiler_config.cpp`: the design's example reads in full; a key left out keeps its
  default; a missing or other version is refused; unknown keys are refused at the top and inside
  `window`; `level`, `window.active`, `buffers.ring_events` and `buffers.linger_ms` out of range
  are refused; a group that is not a string, writer settings that are not an object and a flag
  that is not a bool are refused.
- `window.active` reads a whole number or `"all"`; any other string and `"all"` with a `repeat`
  other than 1 are refused with their paths; `record.keep_periods` reads 0 and a count and is
  absent by default; the `live` keys keep their defaults, read their values and are refused out of
  range; a writer's `enabled` flag is read and removed from its settings, a source's is left in
  them, and a flag that is not a bool is refused.
