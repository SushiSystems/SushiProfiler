# The capture layer: report

Design phase P1 of `docs/design/PROFILER/README.md`, executed inline by the orchestrator on
2026-10-26 from `PLAN.md` in this folder.

## Commits

| Task | Commit | Subject |
| --- | --- | --- |
| 1 | `7da2463` | feat(clock): read the TSC and calibrate it to steady time |
| 2 | `b6a0a8b` | feat(events): define the 24-byte trace event and its kinds |
| 3 | `7560972` | feat(sites): register sites once and filter them at run time |
| 4 | `bb3dcc6` | feat(rings): keep each thread's events in a lock-free ring |
| 5 | `1a280ee` | feat(collector): gate recording and hand every ring to one consumer |
| 6 | `9fc7981` | feat(zones): record zones, counters, values, instants, flows and lanes |
| 7 | `ab18c4c` | feat(lock_wait): record the time a contended lock waits |
| 8 | `aa11a94` | feat(periods): mark periods on named timelines |
| 9 | `d1480e1` | build(bench): build and run the benchmark programs |
| 9 | `56bb4d6` | test(zones): measure the hot path and prove it never allocates |
| 10 | the next commit | docs(profiler): close the capture layer |

## Verification

- `sp test --type debug`: 72 of 72 tests pass, 63 small and 9 medium.
- `python -m pytest tests -q` in `cli/`: 36 of 36 pass.
- `python -I tools/check_all.py .`: 12 checkers clean.
- The allocation test was seen failing: with one `new int` added to its loop,
  `Small_Macros.TheRecordingThreadNeverAllocates` failed; the line was removed before the commit.

## Budget

`sp build --benchmarks --type release`, then `sp bench --type release`, on an AMD Ryzen 5 7600X
under Windows 10 with MSVC. Each value is the median of 31 runs of 1,000,000 iterations after 3
warm-up runs; two runs of the program agreed to within 0.3 ns on every case.

| Case | First run | Second run | Budget |
| --- | --- | --- | --- |
| `zone_compiled_out` | 0.000 ns | 0.000 ns | none |
| `zone_gate_closed` | 1.332 ns | 1.308 ns | under 2 ns: met |
| `zone_site_disabled` | 3.430 ns | 3.434 ns | none |
| `zone_recording` | 13.256 ns | 13.264 ns | under 25 ns: met |
| `counter_recording` | 6.732 ns | 6.948 ns | under 25 ns: met |
| `clock_now` | 6.536 ns | 6.559 ns | none |
| `steady_now` | 16.230 ns | 16.283 ns | none |

A recorded zone costs two TSC reads, 13.1 ns of its 13.3 ns. No allocation after a site's first
use was measured by the counting `operator new` of `tests/capture/zones/allocation_counter.cpp`.
The 1% overhead on a YOLOv5n step is P5's to measure.

## Where the work left the plan

- The process-wide object is `Collector`, not `Hub`: the owner pointed out that `Hub` reads as
  SushiHub. The design pages, the plan and the glossary use the new name.
- The plan put `collector`, `zones`, `lock_wait` and `periods` in the tier of the modules they
  include, which the build refused. `SUSHIPROFILER_SAME_TIER_EDGES` now lists the edges inside a
  tier, as SushiRuntime does, and the layering checker reads them from each module's build file.
- `sushiprofiler_add_module` gained `PRIVATE_DEPENDS`, which the layering checker asked for when
  only `periods/source/` includes `collector` and `rings`.
- `PeriodMark` carries the index of the period it opens instead of its budget, and the budget
  moved to a new kind, `PeriodBudget`. Without the index, a mark made while the gate was closed or
  dropped by a full ring would shift every later period. `EventKind` has 14 kinds.
- Tests that start a thread are `Medium_`, not `Small_`: `ANewThreadOfAClosedCollectorGetsNoRing`,
  `DropsReachTheDrain` and `ARegisteredThreadKeepsItsRingWhileClosed`.
- The allocation test is `Small_Macros.TheRecordingThreadNeverAllocates` in `test_macros.cpp`,
  not `Medium_ZoneAllocations` in a file of its own: the test mirror refuses a test file that
  mirrors no header, and the test uses one thread.
- `zones` gained `admission.hpp`, the one check every record makes, so that `zone.hpp` and
  `records.hpp` share it instead of each holding a copy.
- `record_zone_at` takes an optional interned name, for device spans named per operation.

## Found for P2

A closed gate writes no mark, so the window cannot open the gate from the record. The design now
says P2 gives `PeriodTimeline` an observer declared in the capture tier, which the window
implements (`docs/design/PROFILER/CAPTURE_AND_SESSION.md`, "Periods and the window").

## SOLID breaches and unneeded dependencies

- `Collector` is a singleton, against the default that dependencies arrive through the
  constructor. Kept: a macro at any call site must reach it, and the design names it the one
  process-wide state. Cost: every test that records opens and closes it in its fixture, and two
  test programs cannot record in parallel inside one process.
- The fixture that opens the collector and reads back a thread's events is written four times:
  `tests/capture/zones/recording_fixture.hpp`, and inside `test_collector.cpp`,
  `test_contended_lock.cpp` and `test_period_timeline.cpp`. Cost: a change to how a test drains
  edits four files. A shared test-support folder would remove it; the test mirror has no place
  for one today.
- Unneeded dependencies: none. `check_layering.py` reports no unused or undeclared edge.

## Dispatches

None. Every task ran inline in the orchestrator's session.

## Storage

The scratchpad holds `sp.sh`, under 1 KB. The benchmark build added a Release configuration to
`build/`, the project's own tree.
