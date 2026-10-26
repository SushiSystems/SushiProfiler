# stream_writers

Writers the session feeds one period at a time, in the namespace `SushiProfiler`, so memory holds
one window of periods, not the run. The module depends on `fold`, `record` and `record_writers`,
and privately on `json` and `sites`.

- `SushiProfiler/stream_writers/stream_writer.hpp`: `StreamWriter` is the interface. The session
  calls `begin(record)` once, `write_period(record, period, instances)` once per recorded period
  in order, and `end(record)` once; each returns a `WriteStatus`. `record` is the run so far, and
  `instances` are the zone runs `fold_period` produced for the period, which the record does not
  keep.
- `SushiProfiler/stream_writers/chrome_trace_writer.hpp`: `ChromeTraceWriter(path)` streams the
  run as Chrome trace JSON, which Perfetto and `chrome://tracing` open. The file is one object,
  `{"displayTimeUnit": "ns", "traceEvents": [...]}`, one event per line. Times are microseconds
  since the record's origin.

| Row | Process | Holds |
| --- | --- | --- |
| A thread | 1, `host`; row id the thread index, named by the record's thread name | Zone runs as complete (`X`) events, category the site's group or `zone`; counters and values as `C` events named `<site> (<unit>)`; instants as `i`; flow points as `s`, `t` and `f` with the item as `id` |
| A lane | 1; row ids from 1 000 000, named `<thread> / <lane>` | The zone runs of that lane |
| A timeline | 1; named `timeline <name>` | Each period as an `X` event `<timeline> #<index>`, with `warmup` in its arguments |
| A device track | 2, `devices`; row id the TrackID, named by the track | Spans as `X` events, with `queued_us` and the span's attributes in their arguments |

A path that cannot be opened is `PathUnwritable` from `begin`; every later call then returns
`StreamFailed`.

The tests are in `tests/session/stream_writers/`.
