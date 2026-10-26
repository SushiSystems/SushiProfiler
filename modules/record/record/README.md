# record

The run record: what one profiled run leaves, ready to analyse, compare and check, in the
namespace `SushiProfiler`. It depends on `events` for the ids and on `sites` for `SiteRole` and
`CounterOp`. Nothing in it reads a live program, so a record from disk and a record from a session
are the same value.

- `SushiProfiler/record/run_record.hpp`: `RunRecord`. Its tables are indexed by the collector's
  ids: `names[i]` is NameID `i`, `sites[i]` (a `SiteRecord`) is SiteID `i`, `tracks[i]` is
  TrackID `i`, and index 0 of each is empty. It holds the environment, build and tag facts, the
  `ClockRecord`, one `ThreadRecord` per thread with its dropped, late and unmatched events, the
  periods in order and the `RecordDrops` outside any thread. `name_of`, `site` and `find_name`
  look entries up. Times are nanoseconds since the session's first calibration point. An
  unmatched event is a zone that ends after the zone open around it, or a lane end with no lane
  open.
- `SushiProfiler/record/period_record.hpp`: `PeriodRecord`, one period of a timeline: index,
  timeline, marking thread, begin and end, warm-up flag, budget, `Tag`s, `Fact`s, one `ZoneTree`
  per thread and lane, a `SpanTable` and the `SampleSeries`. A `ZoneTree` lists `ZoneNode`s, each
  after its parent; a node merges every call of one site and name under one parent, with its
  inclusive time, self time and call count. `runs` holds every zone run of an active period as
  a `ZoneRun`, its thread, lane, site, name, begin, end and depth, by thread, lane and begin;
  a warm-up period keeps none.
- `SushiProfiler/record/device_span_keys.hpp`: the keys a device source writes and `roofline`
  reads, in the namespace `SushiProfiler::DeviceSpanKeys`: the span attribute names `K_NODE`,
  `K_GRAPH`, `K_ORIGIN`, `K_INVOCATION`, `K_OPERATION`, `K_LAUNCHED_WIDTH`, `K_LIVE_WIDTH`, `K_DTYPE`, `K_FLOPS` and
  `K_BYTES`, and `peak_flops_fact(track)`, `peak_flops_fact(track, dtype)` and
  `peak_bytes_per_s_fact(track)`, which build the environment fact names `device.<track>.peak_flops`,
  `device.<track>.peak_flops.<dtype>` and `device.<track>.peak_bytes_per_s`. Each key's kind and
  meaning is in the table of `docs/design/PROFILER/CLIENTS.md`. The header is in `record`, below
  both the sources that write the keys and the analyses that read them.
- `SushiProfiler/record/span_table.hpp`: `SpanTable`, a period's `Span`s as columns, plus one
  `AttributeColumn` per attribute key holding integers, reals or interned names. Every column has
  one entry per span: a column added late marks earlier spans absent, and a span added late is
  absent from every column.
- `SushiProfiler/record/sample_series.hpp`: `SampleSeries`, what one counter, value, instant or
  flow site recorded on one thread in a period: times, then values for counters and values, or
  item ids and `FlowPoint`s for flows.

Every type compares field by field with `operator==`, which the round-trip tests rely on. The tests
are in `tests/record/record/`.
