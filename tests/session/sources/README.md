# tests/session/sources

- `test_source.cpp`: a test source begins, adding a track and a name, collects the span of
  period 2 at its record time, and ends; a source that cannot start says why and adds nothing.
- `test_source_context.cpp`: names intern into the session's table; a track is added once and
  keeps its id; times count from the origin and clamp to 0 before it.
- `test_span_sink.cpp`: a valid span and its attributes are stored; spans on an unknown track,
  with an unknown name, ending before they start or queued after they start are refused and
  counted; attributes on a missing row, key 0, an unknown key, of the wrong kind or with an
  unknown name value are refused and counted.
