# The run record, analyses and measurement rules

## The run record

One JSON file, `<name>.sprec.json`, version 1:

- `environment`: host, CPU, OS, device name and driver, SYCL backend, the build id and tag of every
  Sushi library that registered parts, thread counts, the configuration as read;
- `sites`, `names`, `keys`, `tracks`: the interned tables, written once;
- `periods`: per period its index, timeline, begin and end, whether warm-up, the execution layer
  its source reported, and per thread the zone tree (site, name, parent, inclusive, self, calls);
- `spans`: per period, structure of arrays (`queued`, `start`, `end`, `track`, `name`,
  `submit_thread`, `uncertainty`, then each attribute key as a column), so a key's name is written
  once per period, not once per span;
- `samples`: per period, the counters, values, instants and flow points, as columns per site;
- `tags`: the run's tags from the configuration and each period's own;
- `build`: per contributing library, its build type, optimisation and sanitisers;
- `clock`: the clock used, its resolution and its measured cost per read;
- `drops`: per thread the dropped events, the events late for a closed period and the zone and
  lane ends that did not nest; events outside every period, dropped spans, zones open at stop.

Analyses read a record, never the live program, so `sp record show` re-analyses an old record with a
newer analysis.

## Analyses

Generic, in SushiProfiler:

| Name | Answers |
| --- | --- |
| `breakdown` | Per period, self time per group on the timeline's thread, plus time no zone covered |
| `device_idle` | Per period, device busy and idle time; each idle gap attributed to the host zone open on the submitting thread during the gap, else to "launch" if the next span was queued before the gap began, else "unattributed". The record keeps each active period's zone runs; the table adds `blamed_zone` and `blamed_share`, and the cause shares still split gaps into `launch` and `host` |
| `top_spans` | Spans grouped by name and by any attribute the configuration names, with count, total, median and share |
| `short_spans` | Count and total of spans under a threshold (10 µs by default) against the launch floor when one is known |
| `overhead` | Events and bytes recorded, drops, and the cost of the profiler measured against a baseline record |
| `budget` | Per timeline with a budget: used fraction per period, periods over budget, the worst period and its zones |
| `distribution` | Per period time, zone, counter and value: p50, p90, p99, maximum, a log-bucketed histogram, and jitter as the median absolute difference between successive periods |
| `counters` | Per counter site, its operation applied per period (sum, last, maximum, minimum), then the distribution over periods |
| `flows` | Per flow name, across every call site of it, the latency from begin to end and between steps, as a distribution |

In SushiBLAS, `roofline`: per span with `flops` and bytes, achieved FLOP/s and bytes/s, arithmetic
intensity, and the fraction of the bound ceiling from a ceilings file.

Every timing is reported as median, bootstrap 95% interval (2,000 resamples, fixed seed, so the
same record gives the same text), minimum, p95 and coefficient of variation over the active
periods. A steady-state check (MSER-5 over period time) says whether the warm-up was long enough
and recommends a length when it was not.

`sp record compare a b` has a row for every subject both records hold, with the ratio of
medians and its interval, and names a change only when the two intervals do not overlap. It refuses two records whose build environment differs
(optimised against not, sanitised against not) unless told `--allow-build-mismatch`, and it
groups rows by period tags when both records carry them.

Checks turn a record into a verdict. A check is data, not a string to parse:

```json
{ "subject": "timeline:dsp.block", "statistic": "p99", "at_most_ns": 2670000 }
```

`sp record check <record> <checks.json>` exits 1 when a check fails, naming it, and
`check(record, checks)` returns the same verdicts to a C++ test. A library's timing tests and
benchmark gates then read a record instead of timing code by hand.

The record schema is a published, versioned file format with a validator, `sp record validate`. A program
in another language, such as SushiTrack's Python pipeline, writes a record by that schema and every
analysis, comparison and check reads it like any other.

## Measurement rules

1. Device time comes from SYCL event profiling, never from a host clock around a submit.
2. Warm-up periods are excluded by the window and reported with their count.
3. Medians with intervals, never means.
4. A comparison names an improvement only when intervals do not overlap.
5. The record holds the environment in full.
6. The profiler's own cost is in every record that has a baseline.
