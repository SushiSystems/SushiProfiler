# tests/session/sources

- `test_source.cpp`: a test source begins, adding a track and a name, collects the span of
  a period given as a `PeriodSpan` at its record time, and ends; a source that cannot start says why and adds nothing.
- `test_source_context.cpp`: names intern into the session's table; a track is added once and
  keeps its id; times count from the origin and clamp to 0 before it.
- `test_span_sink.cpp`: a valid span and its attributes are stored; spans on an unknown track,
  with an unknown name, ending before they start or queued after they start are refused and
  counted; spans a source drops as late are counted apart from refused ones; attributes on a missing row, key 0, an unknown key, of the wrong kind or with an
  unknown name value are refused and counted.
- `Source::max_delay_ns` is 0 unless a source overrides it. `SpanSink::set_fact` sets and
  replaces a fact on the period, refuses an empty name, and refuses every call on a sink built
  without facts.
