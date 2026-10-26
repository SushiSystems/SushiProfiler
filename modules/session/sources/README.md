# sources

The interface through which spans measured outside the capture path, such as device work, reach
the record, in the namespace `SushiProfiler`. The module depends on `events`, `record` and
`sites`.

- `SushiProfiler/sources/source.hpp`: `Source` is the interface a source implements. `name()`
  returns the name the registry knows it by. `begin(context)` adds its tracks and names, or
  returns a `SourceStatus` saying why it cannot start. `collect(period, sink)` adds every span
  tagged with that period. `end()` releases what `begin` acquired. The session calls `begin` once
  at start, `collect` once per closed period on the consumer thread, and `end` once at stop.
- `SushiProfiler/sources/source_context.hpp`: `SourceContext` is what a starting source may add
  to the record. `intern` returns the record's id for a span name, attribute key or attribute
  value. `add_track` adds a named track, such as one device queue, and returns the same id for a
  name it has seen. `to_record_ns` moves a steady-clock time onto the record's clock, which counts
  from the session's first calibration point; a time before it is 0.
- `SushiProfiler/sources/span_sink.hpp`: `SpanSink` adds a source's spans and attributes to one
  period's `SpanTable`. It refuses and counts a span on an unknown track, with an unknown name or
  with times out of order (`queued_ns <= start_ns <= end_ns`), and an attribute on a missing row,
  on key 0 or an unknown key, with an unknown name value, or of another kind than its column.
  Whatever it accepts is a record `read_record` accepts; `refused()` is the count the session
  adds to the record's dropped spans.

A source that receives its data on another thread, as SushiRuntime's does after each run, queues
it and hands it over in `collect`. A span's period is the followed timeline's current period when
the work was submitted.

The tests are in `tests/session/sources/`.
