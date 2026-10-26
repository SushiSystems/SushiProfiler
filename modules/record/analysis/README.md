# analysis

Named analyses over a run record, in the namespace `SushiProfiler`. Each answers one question and
returns a `Table`, which the record writers print or save. Analyses read the record alone, never
a live program, so an old record can be analysed again with a newer analysis. The module depends
on `record` and `statistics`, and privately on `events` and `sites`.

## The framework

- `SushiProfiler/analysis/table.hpp`: `Table` holds the analysis name, `Column`s (a name and a
  `Unit`: `None`, `Nanoseconds`, `Count` or `Ratio`), rows of `Cell`s and notes on the whole
  table. A `Cell` is empty, text, a whole number or a real.
- `SushiProfiler/analysis/analysis.hpp`: the `Analysis` interface, `name()` and
  `run(record, options)`, and `AnalysisOptions`: the `BootstrapSettings`, `top` (20),
  `group_by`, `short_span_ns` (10 000), `launch_floor_ns` (0 for unknown) and an optional
  `baseline` record.
- `SushiProfiler/analysis/analysis_catalog.hpp`: `AnalysisCatalog` maps names to factories,
  refuses a taken name and lists the names in order. `register_builtin_analyses` adds every
  analysis below.
- `SushiProfiler/analysis/analysis_runner.hpp`: `AnalysisRunner::run(record, names, options)`
  checks every name first. An unknown name refuses the whole run, and the error lists the known
  names; otherwise the result holds one table per name, in the order asked.
- `SushiProfiler/analysis/sample_summary.hpp`: `summarise` gives the `SampleSummary` every
  timing row reports: count, median, its bootstrap interval, minimum, p95 and coefficient of
  variation. `add_summary_columns` and `append_summary` lay it out as columns and cells.
- `SushiProfiler/analysis/subject_samples.hpp`: `subject_samples(record)` returns every subject
  with samples as a `SubjectSamples`: its name, its unit and its samples in period order. The
  names are the ones checks and comparisons use: `timeline:<t>` period times;
  `group:<t>/<group>` self time per period, with `(uncovered)` last; `zone:<site>` inclusive
  time per period that ran the zone or lock wait; `counter:<site>` the fold per period;
  `value:<site>` every sample; `span:<name>` every span length; `flow:<site>` each item's
  begin-to-end latency. An overload takes the periods to read.

## The analyses

Every analysis reads active periods only, the ones the window did not mark as warm-up.

| Name | Rows |
| --- | --- |
| `breakdown` | Per timeline and zone group: the self time per period on the marking thread, summarised, and the group's share of the total period time. Groups are ordered by median, largest first; a last row, `(uncovered)`, holds the time no root zone covered. A zone without a group takes its nearest grouped ancestor's, else `(ungrouped)`. |
| `distribution` | Per timeline (period time), zone and lock wait (inclusive time per period, nested calls of the same site counted once), counter (folded per period) and value (every sample): p50, p90, p99, maximum, jitter and a log2 histogram written as `2^e:count` words. |
| `budget` | Per timeline with a budget: the median used fraction, the number of periods over budget, the worst period's index and fraction, and the zone with the most self time in it. |
| `counters` | Per counter: its operation applied per period (`sum`, `last`, `max`, `min`), then summarised over the periods that hold a sample. |
| `flows` | Per flow: `total`, the latency from an item's begin to its end, and `step`, the latency between successive points of an item, each summarised. Points are taken in time order across periods. |
| `device_idle` | Per track: the idle time per period, summarised, and its share of the period time. Each gap between busy segments is `launch` when the span that ends it was queued before the gap began, else `host`; the time after the last span is `host`. The row gives the share of idle time of each cause. |
| `top_spans` | Spans grouped by name and by the attribute keys in `group_by` (`-` where a span lacks one): count, total, median and share of all span time, ranked by total and cut at `top`, with a note on how many groups were cut. |
| `short_spans` | One row: the spans under `short_span_ns`, their count and time and the shares of each, and their cost at `launch_floor_ns`. A note says when the floor is unknown. |
| `overhead` | Events and bytes recorded, dropped, late and unmatched events, the record's drops and the clock's read cost; with a baseline record, per timeline the two median period times and the cost as their relative difference. Without one, a note says the cost is not measured. |

The design attributes a device gap to the host zone open on the submitting thread during it.
The record keeps zone trees, not zone runs, so `device_idle` splits gaps into `launch` and
`host` only; `docs/design/REMAINING_WORK.md` holds the decision as RW-4.

Inside `source/`, `record_queries.hpp` holds the lookups several analyses share: active periods
and timelines, site and node labels, group inheritance, a site's inclusive time and a counter's
fold, the group series of a timeline and a flow's latencies.

The tests are in `tests/record/analysis/`.
