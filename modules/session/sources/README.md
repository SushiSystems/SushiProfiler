# sources

The interface through which spans measured outside the capture path, such as device work, reach
the record, in the namespace `SushiProfiler`. The module depends on `events`, `record` and
`sites`.

- `SushiProfiler/sources/source.hpp`: `Source` is the interface a source implements. `name()`
  returns the name the registry knows it by. `begin(context)` adds its tracks and names, or
  returns a `SourceStatus` saying why it cannot start. `collect(period, sink)` receives a
  `PeriodSpan` (`index`, `start_steady_ns`, `end_steady_ns`, the period's span on the steady
  clock, end exclusive) and adds every span whose start lies inside it; work that started before
  `start_steady_ns` belongs to a period that has closed, so the source calls `sink.drop_late` for
  it. `end()` releases what `begin` acquired. The session calls `begin` once at start, `collect`
  once per closed period and `end` once at stop, all on the consumer thread. The
  `SourceContext&` that `begin` receives stays valid until `end` returns, and `collect` may call
  `intern`, `add_track` and `to_record_ns` on it without a lock.
- `SushiProfiler/sources/source_context.hpp`: `SourceContext` is what a starting source may add
  to the record. `intern` returns the record's id for a span name, attribute key or attribute
  value. `add_track` adds a named track, such as one device queue, and returns the same id for a
  name it has seen. `to_record_ns` moves a steady-clock time onto the record's clock, which counts
  from the session's first calibration point; a time before it is 0.
- `SushiProfiler/sources/span_sink.hpp`: `SpanSink` adds a source's spans and attributes to one
  period's `SpanTable`. It refuses and counts a span on an unknown track, with an unknown name or
  with times out of order (`queued_ns <= start_ns <= end_ns`), and an attribute on a missing row,
  on key 0 or an unknown key, with an unknown name value, or of another kind than its column.
  Whatever it accepts is a record `read_record` accepts. `drop_late(count)` counts spans a source
  dropped because their period had closed. The session adds `refused()` and `late()` to the
  record's `drops.spans_dropped`, which counts the whole run; a period has no count of its own.

A source that receives its data on another thread, as SushiRuntime's does after each run, queues
it and hands it over in `collect`. A span's period is the one whose steady span holds the work's
start time. `consumer.linger_ms` is how long a closed period waits for late work.

The tests are in `tests/session/sources/`.
