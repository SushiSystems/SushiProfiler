# record

The run record: what one profiled run leaves, ready to analyse, compare and check, in the
namespace `SushiProfiler`. It depends on `events` for the ids and on `sites` for `SiteRole` and
`CounterOp`. Nothing in it reads a live program, so a record from disk and a record from a session
are the same value.

- `SushiProfiler/record/run_record.hpp`: `RunRecord`. Its tables are indexed by the collector's
  ids: `names[i]` is NameID `i`, `sites[i]` (a `SiteRecord`) is SiteID `i`, `tracks[i]` is
  TrackID `i`, and index 0 of each is empty. It holds the environment, build and tag facts, the
  `ClockRecord`, one `ThreadRecord` per thread with its dropped and late events, the periods in
  order and the `RecordDrops` outside any thread. `name_of`, `site` and `find_name` look entries
  up. Times are nanoseconds since the session's first calibration point.
- `SushiProfiler/record/period_record.hpp`: `PeriodRecord`, one period of a timeline: index,
  timeline, marking thread, begin and end, warm-up flag, budget, `Tag`s, `Fact`s, one `ZoneTree`
  per thread and lane, a `SpanTable` and the `SampleSeries`. A `ZoneTree` lists `ZoneNode`s, each
  after its parent; a node merges every call of one site and name under one parent, with its
  inclusive time, self time and call count.
- `SushiProfiler/record/span_table.hpp`: `SpanTable`, a period's `Span`s as columns, plus one
  `AttributeColumn` per attribute key holding integers, reals or interned names. Every column has
  one entry per span: a column added late marks earlier spans absent, and a span added late is
  absent from every column.
- `SushiProfiler/record/sample_series.hpp`: `SampleSeries`, what one counter, value, instant or
  flow site recorded on one thread in a period: times, then values for counters and values, or
  item ids and `FlowPoint`s for flows.

Every type compares field by field with `operator==`, which the round-trip tests rely on. The tests
are in `tests/record/record/`.
