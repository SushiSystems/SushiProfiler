# The session, the run record and the record tools: report

Design phase P2 of `docs/design/PROFILER/README.md`, executed inline by the orchestrator on
2026-10-26 from `PLAN.md` in this folder.

## Commits

| Task | Commit | Subject |
| --- | --- | --- |
| 1 | `96d3471` | feat(json): read and write JSON with 64-bit integers kept exact |
| 1 | `dae4f88` | refactor(json): keep the surrogate refusal under the column limit |
| 2 | `abac3e2` | feat(periods): let an observer open and close the gate at a mark |
| 2 | `230583b` | refactor(capture): keep three lines under the column limit |
| 3 | `11cfb20` | feat(record): define the run record |
| 4 | `47d4892` | feat(record_json): write and read the run record as versioned JSON |
| 5 | `d1f40ab` | feat(statistics): summarise samples with medians, intervals and a steady-state check |
| 6 | `bc77dfa` | feat(analysis): run named analyses over a record |
| 6 | `dba55d8` | feat(analysis): analyse device spans |
| 7 | `3ba2129` | feat(record_writers): print analyses and write the record file |
| 8 | `9d890d5` | feat(analysis): name every subject's samples for checks and comparisons |
| 8 | `367d6af` | feat(compare): compare records row by row with intervals |
| 8 | `d6fbc25` | docs(compare): keep the comparison's comment block within eight lines |
| 9 | `f05c249` | feat(checks): turn a record into pass or fail verdicts |
| 10 | `5600313` | refactor(json): share the strict field reader between file readers |
| 10 | `b815d06` | feat(config): read the profiler configuration |
| 11 | `f5e9405` | feat(window): record only the scheduled periods of a timeline |
| 12 | `ea53ee5` | feat(sources): let a source add spans to a period |
| 12 | `1737e59` | feat(environment): record the host and what each library contributes |
| 13 | `197e3c8` | feat(record): count each thread's events that did not nest |
| 13 | `ac24f26` | test(analysis): shift the overhead baseline rows past the unmatched row |
| 13 | `e204746` | feat(fold): fold a period's events into zone trees and samples |
| 14 | `d36427f` | feat(stream_writers): stream each period into a Chrome trace |
| 15 | `7cda1c6` | feat(registry): make sources and writers by name |
| 15 | `0ffbe6b` | feat(consumer): place drained events in periods and close them in order |
| 15 | `f2b94c0` | feat(session): record a run from a configuration |
| 16 | `d1bdbb5` | fix(analysis): join the call sites of one flow |
| 16 | `95c5d82` | feat(record_tool): show, compare, check and validate records |
| 17 | `95e94a9` | test(session): compare and check the end-to-end record |
| 17 | `061d971` | feat(consumer): count drains and measure the idle drainer |
| 17 | the next commit | docs(profiler): close the session and the run record |

## Verification

- `sp test --type debug`: 264 of 264 tests pass, 242 small and 22 medium.
- `python -m pytest -q` in `cli/`: 42 of 42 pass.
- `python -I tools/check_all.py .`: 12 checkers clean.
- `Medium_Session.RecordsTenPeriodsFromTwoThreads` covers the plan's goal in one test: a session
  records ten periods of zones, counters, values, instants and flows on two threads; the terminal
  prints the analyses; the record file reads back equal; the Chrome trace parses; the `flows`
  table has rows; the record compares with itself; one check fails and one passes. It passed in
  six runs after `f2b94c0` and in three after `95e94a9`.
- `sp record validate`, `show` and `check` ran on the record that test writes: the record is
  valid with 10 periods and 2 threads, `show` printed the tables, and a check of a 1 ns bound
  printed `FAIL` and exited 1.
- From commit `ac24f26` on, every commit went through one script that builds, requires every test
  to pass and every checker to be clean, and only then commits. Before it, `197e3c8` landed with
  `Small_OverheadAnalysis` failing, because the checkers were run and the tests were not.

## Budget

`sp build --benchmarks --type debug`, then `sp bench --type debug`, on the same machine as P1.
`sp_bench_consumer` lets a consumer idle for 2 s and counts its drains over the next 8 s.

| Case | Measured | Budget |
| --- | --- | --- |
| Drainer while idle | 1.000 wake-ups a second | under one a second: at the bound, not under it |

An idle consumer doubles its wait from 1 ms to the 1 s cap within about one second and then
drains once a second. Meeting "under one" needs a cap above 1 s, which lengthens the time a
burst after a quiet stretch waits before its first drain, and so raises the chance a ring fills.
The cap stays at 1 s; the owner decides whether the budget reads "at most one".

## Where the work left the plan

- Builds ran with 5 jobs, as the owner asked during the work, not 4.
- The strict field reader moved from `record_json` into `json` as `JsonFieldReader`, because the
  configuration reader needs the same refusals with a different root name.
- The record gained `ThreadRecord.unmatched`: zone and lane ends with no open begin, and zones
  that cross their parent. `RECORD_FORMAT.md` documents it; the format version stays 1, since no
  record was written before it.
- `compare` rows come from `subject_samples`, every subject a check can name, instead of the four
  row kinds of the plan. The column is `change` with `faster`, `slower`, `lower` or `higher`, not
  a `changed` flag.
- Check subjects include `group:` beside the six of the plan.
- The poll rule is the design's, made exact: 1 ms over half full, the same wait between a quarter
  and a half, and double the wait, up to 1 s, under a quarter.
- A period without its end mark at stop is not kept, and its events count as outside.
- A lane end with no lane open is ignored and counted as unmatched for its thread.
- One session runs per process; a second start names the session that is running.
- The record tool is a module plus a program; the program folder is the repository's first under
  `applications/`, built when `SP_BUILD_APPLICATIONS` is on, which it is for a top-level build.
- `flows` and the `flow:` subjects found no item with both ends, because a flow's begin and end
  sit at different call sites and each call site is its own site. Running `sp record show` on a
  real record exposed it; `d1bdbb5` fixed it with a regression test seen failing first.
- `Consumer::drains()` and `sp_bench_consumer` were added at the close, because the design asks
  for the idle-drainer budget to be measured before P2 closes.

## Found for later

- `device_idle` splits a device gap into `launch` and `host` only. The design attributes a gap to
  the host zone open on the submitting thread, but the record keeps zone trees, not runs, so
  nothing in it says which zone was open at an instant. Both ways out change the record format.
  This is RW-4, waiting for the owner.
- `sp record`, `sp bench` and `sp test` take the build type from the tree's cache when `--type` is
  not given. After `sp build --type debug` the cache of this tree still read `Release`, so
  `sp record` without `--type` looked for the program in `Release/` and said to build first.

## SOLID breaches and unneeded dependencies

- `Collector` is still a singleton. `Session::start` reaches it through `Collector::instance()`;
  `Consumer` and `PeriodAssembler` take it, or its tables, by reference. Cost: two sessions cannot
  run in one process, which the session states and enforces with a static slot.
- The session's one-per-process slot is global state of its own, for the same reason.
- The setup that closes the collector, drains the leftovers and opens it again is now written in
  `test_consumer.cpp`, `test_session.cpp`, `test_window.cpp` and `benchmark_consumer.cpp`, beside
  the four copies P1 left. Cost: a change to how a test opens the collector edits every copy.
- `show_command.cpp` builds its own catalog with `register_builtin_analyses`, so the tool shows
  the built-in analyses only. A library that adds an analysis needs its own tool or a registry
  the tool can be given.
- Unneeded dependencies: none. `check_layering.py` reports no unused or undeclared edge.

## Dispatches

None. Every task ran inline in the orchestrator's session.

## Storage

The scratchpad holds the build and gate scripts, under 120 KB. The benchmark build added
`build/benchmarks/Debug/` and the tool `build/applications/Debug/`, both in the project's own tree.
