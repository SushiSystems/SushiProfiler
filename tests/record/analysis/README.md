# tests/record/analysis

`analysis_record.hpp` builds the record the analysis tests share and compares rows cell by cell,
reals within 1e-9 relative. Its timeline `step` runs on thread 0:

| Period | Length | Zones (inclusive, self) | Samples |
| --- | --- | --- | --- |
| 0, warm-up | 100 | forward (99, 99) | none |
| 1 | 100 | forward (60, 40) > conv (20, 20); load (30, 30) | bytes 100, 50; peak 5, 9, 3; loss 1, 3; item 1 begins at 110, steps at 130 |
| 2 | 120 | forward (80, 50) > conv (30, 30); load (20, 20) | bytes 200; peak 4; loss 2; item 1 ends at 210, item 2 begins at 250 |
| 3 | 110 | forward (70, 70); load (30, 30) | peak 7; item 2 steps at 330, ends at 340 |

`forward` is in group `Model`, `conv` has no group and inherits it, and `load` is in `Data`.
`bytes` is a sum and `peak` a maximum, both in `B`. Active periods have a 105 ns budget. The
thread dropped 2 events, was late with 1 and had 4 unmatched; the record's drops are 1, 2 and 3.

`span_record()` holds one timeline on track `queue 0`, a warm-up period with one `gemm` span,
and two active periods of 1000 ns. Times are queued, start and end:

| Period | Spans |
| --- | --- |
| 1, 1000 to 2000 | `gemm` 1000, 1100, 1400 with `batch` 4; `copy` 1000, 1400, 1405; `gemm` 1500, 1600, 1900 with `batch` 8 |
| 2, 2000 to 3000 | `gemm` 1950, 2000, 2600 with `batch` 4; `copy` 2590, 2700, 2708 |

- `test_table.cpp`: a cell holds one kind; cells of different kinds differ; columns by name.
- `test_analysis.cpp`: the options' defaults; an analysis runs through the interface.
- `test_analysis_catalog.cpp`: made by name; a taken name refused; the built-ins in order, `roofline` among them.
- `test_analysis_runner.cpp`: tables in the order asked; an unknown name refuses the run and
  lists the known names.
- `test_sample_summary.cpp`: the summary of fixed samples; zeros for none; columns and cells line
  up.
- `test_subject_samples.cpp`: the eleven host subjects of the shared record in order, and the
  span subjects of the span record.
- `test_breakdown_analysis.cpp`: `Model` 60, 80, 70; `Data` 30, 20, 30; uncovered 10, 20, 10;
  shares over 330 ns.
- `test_distribution_analysis.cpp`: the rows of the timeline, three zones, two counters and the
  value, with percentiles, jitter and histogram words.
- `test_budget_analysis.cpp`: two periods over budget, the worst is period 2 at 120/105 with
  `forward`; no budget, no row.
- `test_counters_analysis.cpp`: `bytes` sums to 150 and 200; `peak` peaks at 9, 4 and 7.
- `test_flows_analysis.cpp`: totals 100 and 90; steps 20, 80, 80 and 10.
- `test_device_idle_analysis.cpp`: idle 395 and 392 ns; launch gaps 100 and 100, host gaps 295
  and 292, all `unattributed` with no zone runs; with runs, the innermost zone open on the
  submitting thread at the gap's start is named, a zone on another thread is not, and a queued
  span's gap is `launch`; no spans, no rows.
- `test_top_spans_analysis.cpp`: `gemm` 1200 ns over three spans before `copy` 13 over two; by
  `batch` and cut at two, `gemm` 4 and `gemm` 8 with one group not shown.
- `test_roofline_analysis.cpp`: two `gemm` spans of 100 us at 5e7 FLOP and 2e6 B reach 5e11 FLOP/s
  and 2e10 B/s at intensity 25, half of the 1e12 compute roof; a `copy` span reaches 1e10 FLOP/s at
  intensity 0.125, 0.8 of the 1.25e10 memory bound; a track without ceiling facts has empty
  ceiling cells; two spans without both attributes are counted and warm-up periods are skipped; a
  ceiling fact that is not a number counts as missing. With `node` and `dtype` attributes, node 7
  merges two `f16` spans against the 4e12 `f16` ceiling (0.25), node 8 splits into an `f16` row
  (0.125) and an `f32` row on the type-less 1e12 ceiling (1.0), and a span with no FLOPs and 8e6 B
  is `memory` at 0.8 of the memory ceiling. With `graph` and `origin` attributes on `gemm` spans of node 3, `g1` and `g2` give separate rows, `g1` with `o1` (two spans, 2e8 FLOP) and with `o2` (one span) are two rows, and the span with neither keeps an empty-labelled row.
- `test_short_spans_analysis.cpp`: two of five spans under 10 ns, 13 ns of 1213; priced at a
  20 ns floor, 40 ns.
- `test_overhead_analysis.cpp`: 33 events and 792 bytes, every drop, the note without a
  baseline, and a 10% cost against a baseline of 90, 100 and 110 ns.

The bootstrap ends in the expected rows come from a Python reimplementation of the generator.

- `test_period_breakdown.cpp`: a node takes the nearest grouped ancestor's group, else
  `(ungrouped)`; groups come largest self time first; only the marking thread's roots count as
  covered; uncovered time is 0 when zones overrun the period; every thread's totals sum its lanes;
  the groups equal the `breakdown` rows of a one-period record.
