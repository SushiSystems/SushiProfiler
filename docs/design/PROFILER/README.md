# SushiProfiler

**Status:** Open — approved by the owner on 2026-10-26; P1 built 2026-10-26, P2 built 2026-10-26, P2b is next.
**About:** A general-purpose profiler for any C++17 program: zones, counters, values, flows and device spans over marked periods, kept as a run record that is analysed, compared and checked.
**Modules:** `modules/foundation/clock`, `modules/foundation/events`, `modules/capture/sites`, `modules/capture/rings`, `modules/capture/collector`, `modules/capture/zones`, `modules/capture/lock_wait`, `modules/capture/periods`, `modules/foundation/json`, `modules/record/record`, `modules/record/record_json`, `modules/record/statistics`, `modules/record/analysis`, `modules/record/record_writers`, `modules/record/compare`, `modules/record/checks`, `modules/session/config`, `modules/session/window`, `modules/session/sources`, `modules/session/environment`, `modules/session/fold`, `modules/session/stream_writers`, `modules/session/registry`, `modules/session/consumer`, `modules/session/session`, `modules/tools/record_tool`

## Problem

No library of the stack measures where its time goes in a way that is kept and compared.
SushiAI trains YOLOv5n at about 5 s per iteration on an RTX 3080 Ti at 20-46% GPU use, and
nothing says whether the kernels, the host or the waits between them cost the time. SushiRuntime
records device timestamps that nothing reads; SushiBLAS counts no FLOPs; SushiDSP keeps the
fastest of 200 repetitions, which hides the slow blocks behind dropouts; SushiTrack prints a mean
with no date, commit or machine. SushiEngine has a finished trace layer, but it sits above
SushiRuntime, carries engine dependencies and is licensed apart from the rest of the stack.
[STACK_SURVEY.md](STACK_SURVEY.md) holds what each repository measured on 2026-10-26.

## Decision

The owner's rulings of 2026-10-26:

1. The profiler serves every library, not SushiAI alone; what SushiRuntime and SushiBLAS gain
   stays there.
2. SushiEngine's trace layer moves into this repository, under PolyForm Noncommercial 1.0.0, and
   becomes the profiler of every Sushi library.
3. SushiProfiler is general purpose and configurable. It is shaped by no client: any C++17
   program, in the stack or outside it, profiles itself with it, and it knows no model, kernel,
   frame or Sushi library.
4. It is optimised and does not carry over the engine's defects
   ([ENGINE_CHANGES.md](ENGINE_CHANGES.md)).
5. SushiDSP's and SushiTrack's adoption are phases of this design.

Calls made by the orchestrator, which the owner left to it:

- The name SushiProfiler, alias `sp`; macros with each library's own prefix above it (`SR_ZONE`,
  `SB_ZONE`, `SA_ZONE`).
- No dependency but the C++17 standard library and Threads; SushiProfiler logs nothing and returns
  statuses.
- `FrameProfiler`, with the engine's script-module ledger, stays in the engine as a consumer.
- One complete event per zone instead of a begin and an end.
- Counters, values, instants, external times, flows and lanes are event kinds of the one ring.
- Chrome trace JSON for the timeline; a JSON run record with columns for spans; a binary format
  waits until a measured record is too large.
- The configuration refuses unknown keys; checks are JSON data, not an expression language.
- MSER-5 for the steady-state check; a fixed-seed bootstrap for intervals.
- SushiProfiler is an optional dependency, on by default, in every library that uses it.
- SushiRuntime's `RunReport` gains a node table, an additive change.
- SushiAI's findings come from two runs compared: one on the production execution path, one with
  runtime profiling on.
- Python reaches SushiProfiler through the published record schema; a binding waits for P9.

The literature and tools behind these calls are in [PRIOR_ART.md](PRIOR_ART.md).

## Design

The tiers, lowest first, each depending only on those before it:

| Tier | Holds |
| --- | --- |
| foundation | The clock and its calibration; the event types; JSON |
| capture | Sites and their registry; per-thread rings; the process-wide collector; zones and the other event kinds; periods |
| record | The run record and its JSON form; statistics; analyses and their runner; record writers; comparison; checks |
| session | Configuration; the registry of parts; the window; sources; environment contributors; the fold; the consumer thread; stream writers; `Session` |
| tools | The record tool behind `sp record show`, `compare`, `check` and `validate` |

The record tier does not depend on the session tier, so a record on disk is analysed with no
session running.

The detail is in four pages:

- [CAPTURE_AND_SESSION.md](CAPTURE_AND_SESSION.md): the modules, the event layout, the hot path,
  the event kinds beyond zones, periods and the window, the session, sources, configuration.
- [RECORD_AND_ANALYSES.md](RECORD_AND_ANALYSES.md): the run record, the analyses, checks, and the
  measurement rules.
- [CLIENTS.md](CLIENTS.md): what SushiRuntime, SushiBLAS and SushiAI add at their own tiers.
- [ENGINE_CHANGES.md](ENGINE_CHANGES.md): each change from SushiEngine's trace layer and the
  defect it removes.

Performance budget, measured by benchmarks in this repository before P2 closes:

| Case | Budget |
| --- | --- |
| Zone, compiled at level 0 | No instruction |
| Zone, gate closed | One relaxed load and one branch; under 2 ns |
| Zone, recording | Under 25 ns: two clock reads and one 24-byte store |
| Zone thread | No allocation after `register_this_thread` |
| Drainer while idle | Under one wake-up a second |
| SushiAI YOLOv5n step with stage zones on | Under 1% slower than with the profiler off |

P1 met the zone rows. P2 measured the idle drainer at one wake-up a second, the bound itself;
P2b writes the row as "at most one" (RW-5). The YOLOv5n row is P5's to measure.

## Phases

| Id | Phase | State |
| --- | --- | --- |
| P1 | SushiProfiler: `clock`, `events`, `sites`, `rings`, `collector`, `zones`, `lock_wait`, `periods`; the zone, ring and allocation tests; the budget benchmarks | built |
| P2 | SushiProfiler: `config`, `registry`, `window`, `sources`, `environment`, `fold`, `consumer`, `session`, `record`, `statistics`, `analysis`, the writers, `compare`, checks, `sp record show / compare / check / validate` | built |
| P2b | SushiProfiler v0.2.0: the shared library, the source contract (period span, context lifetime, late work), the registration pattern, zone runs for `device_idle` | planned |
| P3 | SushiRuntime: the device source, the node table, the runtime's own zones | planned |
| P4 | SushiBLAS: cost rules, `device_ceilings`, `roofline`, the benchmark harness on a timeline | planned |
| P5 | SushiAI: stage zones, timelines, attribution, the run-file section, `sa profile` | planned |
| P6 | SushiAI: where a YOLOv5n step goes, written up from records | planned |
| P7 | SushiDSP: the benchmark harness on a budget timeline, solver counters, checks in place of hand-timed tests | planned |
| P8 | SushiTrack: a period per frame with zones and counters per stage; benchmark tables from records | planned |
| P9 | A C ABI and a Python record writer; a Tracy consumer; what-if from the dependency graph; a memory timeline; hardware counters | planned |

Each phase lands with its tests, module READMEs and changelog lines in the repository it names,
and each repository releases before the one above pins it. P7 and P8 wait on P2's release only.
SushiEngine's move onto SushiProfiler is proposed to the session working there, with a shim from
`SE_ZONE` to `SP_ZONE` that keeps the editor's Profiler window working.

The acceptance criteria and the design record of the approval are in SushiAI's work folder
`docs/agent/2026_10_26_PROFILER/SPEC.md`.

## Open questions

- Whether SushiRuntime's registration index counts every task SushiAI's lowering emits, including
  tasks SushiBLAS records for scratch or late pairs. P5's attribution test answers it; if not,
  lowering passes the IR node id in the task description, a SushiBLAS change.
- Whether a period on SushiRuntime's command-graph replay path can be timed as one span, so an
  unprofiled run still has a device total.
- Static or shared library. It builds static, the orchestrator's proposal, because the hot path is
  inline and every client embeds it; a shared build would need the collector exported once.
