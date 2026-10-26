# The session, the run record and the record tools: implementation plan

**Status:** Open — written 2026-10-26; executed inline by the orchestrator, as the owner chose.
**Implements:** PROFILER#P2

> For agentic workers: REQUIRED SUB-SKILL: superpowers:executing-plans. Steps use checkbox
> (`- [ ]`) syntax for tracking.

**Goal:** A program that marks periods and records zones, counters, values, instants and flows gets
a run record on disk, a terminal summary and a Chrome trace; two records compare; a check over a
record passes or fails; a record round-trips through its JSON form and validates.

**Architecture:** Three tiers above capture. `record` holds the run record, its JSON form,
statistics, analyses, record writers, comparison and checks, and reads no live program. `session`
holds the configuration, the registry of parts, the window, sources, environment contributors, the
fold, stream writers, the consumer thread and `Session`. `tools` holds the record tool behind
`sp record`. A JSON reader and writer of our own sit in `foundation`.

**Tech stack:** C++17, the standard library and Threads; GoogleTest for tests; Python 3.11 and
Typer for the CLI.

**Spec:** `docs/design/PROFILER/README.md` with `CAPTURE_AND_SESSION.md` and
`RECORD_AND_ANALYSES.md`.

## Global constraints

- The standard library and Threads only; no logging; every failure is a returned status.
- One home per fact: module READMEs carry module facts; the design carries intent.
- Every task: tests first, seen failing; `sp build`, `sp test` and `tools/check_all.py` clean
  before its commit; module README, umbrella header, `ARCHITECTURE.md` table and one changelog line
  in the same commit.
- Builds run with 4 jobs through `sp`.
- A record is deterministic: the same events give the same bytes; the bootstrap uses a fixed seed
  and a portable index mapping, never `std::uniform_int_distribution`.

## Calls made for this plan

1. **JSON of our own** (`foundation/json`): a value type, a reader with line and column in its
   errors and a depth limit, and a streaming writer. Integers keep 64 bits; doubles are written
   shortest round-trip with `std::to_chars`.
2. **Gate and lifetime split.** `Collector::open`/`close` start and end a session; `pause` and
   `resume` move the gate inside it without discarding ring contents.
3. **Period observer.** `PeriodObserver` in `periods` has `before_mark` and `after_mark`;
   `PeriodTimeline::observe` installs one. `mark` calls `before_mark`, writes its event if the
   gate is open, then calls `after_mark`. The window opens the gate in `before_mark` when the
   opening period is recorded and closes it in `after_mark` when it is not, so the last recorded
   period still gets its end mark.
4. **Period membership.** A zone, sample or flow point belongs to the period of the configured
   timeline whose span holds its begin time. A period closes once a drain's calibration point is
   `linger_ms` (default 100) past its end, or at stop. An event that begins in a closed period is
   counted as late for its thread. With no timeline configured the session is one period.
5. **Times in the record** are nanoseconds since the session's first calibration point.
6. **Record ids** are the collector's: `names[i]` is NameID `i`, `sites[i]` is SiteID `i`.
7. **Zone trees** per period, per thread, per lane: node `{site, name, parent, inclusive_ns,
   self_ns, calls}`, siblings merged by `(site, name)`.
8. **Analysis output** is a generic `Table` of named, unit-carrying columns, so writers render any
   analysis without knowing it.
9. **Percentiles** interpolate between closest ranks (type 7). **MSER-5** over batch means of 5
   periods. **Histograms** have one bucket per power of two.
10. **`sp record show | compare | check | validate`**: a group, because `sp check` already runs the
    repository's checkers. The design's names change to these.

## Review focus

1. A record with a period that has no zones, spans or samples writes and reads back.
2. A zone that begins in one period and ends in the next is counted in the first.
3. Two runs of the same events produce byte-identical records and identical analysis text.
4. The reader refuses an unknown key and an out-of-range value, naming the key and its place.
5. A window of `wait` periods records nothing during them, and the last active period has its end.

---

## Task 1: `json` (foundation)

Files: `modules/foundation/json/` with `json_value.hpp`, `json_reader.hpp`, `json_writer.hpp`,
sources; tests mirror.

- `JsonValue`: null, bool, integer (`std::int64_t`, plus `std::uint64_t` above `INT64_MAX`),
  real, string, array, object kept in insertion order; typed accessors returning pointers.
- `JsonReadResult read_json(std::string_view)`: value or `{line, column, message}`; depth 128.
- `JsonWriter`: `begin_object`, `key`, `value(...)`, `end_object`, arrays, `pretty` option;
  writes to `std::string`.
- Tests: round trip of every kind; `-0.0`, `1e300`, `2^64-1`; escapes and `\u` pairs; errors name
  line and column; depth limit; unknown literal refused.
- Commit `feat(json): read and write JSON with 64-bit integers kept exact`.

## Task 2: Gate pause and period observer (capture)

- `Collector::pause()`, `resume()`; `is_open()` reports the session, `recording()` the gate.
- `PeriodObserver { virtual void before_mark(const PeriodTimeline&, std::uint64_t index) noexcept; virtual void after_mark(...) noexcept; }`,
  `PeriodTimeline::observe(PeriodObserver*) noexcept`.
- Tests: pause keeps ring contents; an observer sees both calls around the write; a mark that
  closes the gate in `after_mark` is still written.
- Commit `feat(periods): let an observer open and close the gate at a mark`.

## Task 3: `record` (record)

The run record model: environment facts, build facts, clock facts, sites, names, threads,
timelines, periods (index, timeline, begin, end, warm-up, budget, tags, facts, zone trees per
thread and lane, span table with attribute columns, sample series per site), drops (per thread:
dropped, late; zones open at stop; spans dropped). Plain structs with `operator==`.

- Tests: equality, a span table's columns stay aligned when an attribute is absent on some spans.
- Commit `feat(record): define the run record`.

## Task 4: `record_json` (record)

`write_record(const RunRecord&) -> std::string`, `read_record(std::string_view) -> RecordReadResult`
refusing unknown keys and out-of-range values with a JSON path; `"sushiprofiler_record": 1`.
`docs/reference/RECORD_FORMAT.md` documents the format.

- Tests: a full record round-trips byte for byte; an empty period round-trips; each refusal names
  its path; a newer version is refused.
- Commit `feat(record_json): write and read the run record as versioned JSON`.

## Task 5: `statistics` (record)

`median`, `percentile(p)`, `minimum`, `maximum`, `coefficient_of_variation`,
`bootstrap_median_interval(samples, resamples=2000, confidence=0.95, seed)`, `mser5`,
`log2_histogram`, `jitter` (median absolute successive difference). Each takes a span of doubles.

- Tests with fixed inputs and exact expected values; the bootstrap gives the same interval twice
  and on the known seed a pinned value.
- Commit `feat(statistics): summarise samples with medians, intervals and a steady-state check`.

## Task 6: `analysis` (record)

`Table`, `Column`, `Cell`; `Analysis` interface (`name`, `run(const RunRecord&, const AnalysisOptions&) -> Table`);
`AnalysisCatalog` (name to factory; `register_builtin_analyses`); `AnalysisRunner`. One file per
analysis: `breakdown`, `distribution`, `budget`, `counters`, `flows`, `device_idle`, `top_spans`,
`short_spans`, `overhead`.

- Tests per analysis on a hand-built record with exact expected rows; an unknown name is refused
  with the known names.
- Commits: `feat(analysis): run named analyses over a record` (interface, catalog, runner,
  host analyses) and `feat(analysis): analyse device spans` (span analyses).

## Task 7: `record_writers` (record)

`RecordWriter` (`write(const RunRecord&, const std::vector<Table>&) -> WriteStatus`): `terminal`
(aligned text to a stream, top N rows), `record` (the JSON file).

- Commit `feat(record_writers): print analyses and write the record file`.

## Task 8: `compare` (record)

Rows: period time per timeline, self time per group, zone inclusive time per site, counters.
Each row: both medians, the ratio of medians with a bootstrap interval, `changed` only when the two
records' intervals do not overlap. Refuses differing build facts unless allowed; groups by period
tags when both records carry them.

- Commit `feat(compare): compare records row by row with intervals`.

## Task 9: `checks` (record)

`Check {subject, statistic, at_most?, at_least?}` read from JSON; subjects `timeline:`, `zone:`,
`span:`, `counter:`, `value:`, `flow:`; statistics `median`, `p50`, `p90`, `p95`, `p99`, `min`,
`max`; `_ns` bounds for times. `check(record, checks) -> std::vector<Verdict>`.

- Commit `feat(checks): turn a record into pass or fail verdicts`.

## Task 10: `config` (session)

`ProfilerConfig` and `read_profiler_config(std::string_view)`, the JSON of the design plus
`buffers.linger_ms`; refuses unknown keys and out-of-range values. `docs/reference/CONFIG_FORMAT.md`.

- Commit `feat(config): read the profiler configuration`.

## Task 11: `window` (session)

`Window(schedule)` implements `PeriodObserver`: `phase_of(index)` is skipped, waiting, warm-up,
active or done; it resumes and pauses the collector as call 3 says.

- Commit `feat(window): record the scheduled periods of one timeline`.

## Task 12: `sources` and `environment` (session)

`Source`, `SourceContext` (tracks, keys, the calibration), `SpanSink`; `EnvironmentContributor`
and the host facts (OS family, CPU brand from CPUID, hardware threads, compiler, build type).

- Commit `feat(sources): let a source add spans to a period` and
  `feat(environment): record the host and what each library contributes`.

## Task 13: `fold` (session)

Events of one period to zone trees per thread and lane, sample series, flow points and zone
instances for stream writers; self time from children; late and unmatched events counted.

- Commit `feat(fold): fold a period's events into zone trees and samples`.

## Task 14: `stream_writers` (session)

`StreamWriter` (`begin`, `write_period`, `end`); `chrome_trace`: complete events per zone
instance, counters, instants, flows, thread and lane names, device tracks.

- Commit `feat(stream_writers): stream each period into a Chrome trace`.

## Task 15: `registry`, `consumer` and `session` (session)

`Registry` of factories by name; the consumer thread with the adaptive poll; `Session::start`,
`timeline`, `stop`.

- Test: a program marks 10 periods with zones, counters, values, instants and flows on two threads;
  the record holds them, the terminal output lists the analyses, the Chrome trace parses.
- Commits per module.

## Task 16: The record tool and `sp record`

`modules/tools/record_tool` (`run_record_tool(arguments, out, err) -> int`) and the program
`applications/sp_record/`; the CLI group `sp record show | compare | check | validate`.

- Commit `feat(record_tool): show, compare, check and validate records`.

## Task 17: Close P2

Design status P2 `built`; glossary; architecture; `REMAINING_WORK.md` drops RW-2; REPORT.md;
the work folder goes in the close commit.
