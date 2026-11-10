# tests/session/config

- `test_profiler_config.cpp`: every section of an example with no default value reads into its
  field; a key left out keeps the struct's default; a missing or other version is refused; rows of
  one test refuse an unknown key at the top, inside `window` and inside `record`; `level`,
  `window.active`, `buffers.ring_events` and `buffers.linger_ms` out of range are refused; a group
  that is not a string, writer settings that are not an object and a flag that is not a bool are
  refused.
- `window.active` reads a whole number or `"all"`; any other string is refused with its path; rows
  of one test refuse `"all"` with a `repeat` of 2 and accept it with a `repeat` of 1;
  `record.keep_periods` reads 0 and a count, and its absence keeps everything; the `live` keys read
  their values and are refused out of range; a writer's `enabled` flag is read and removed from its
  settings, a source's is left in them, and a flag that is not a bool is refused.
