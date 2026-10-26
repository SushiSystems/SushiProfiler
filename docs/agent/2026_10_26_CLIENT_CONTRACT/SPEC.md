# SushiProfiler v0.2.0: the contract a client library needs

**Status:** Open — written 2026-10-26; decided by the orchestrator on the owner's delegation, nothing built yet.

**Implements:** PROFILER#P2b

The three wave-1 analyses (SushiRuntime, SushiDSP and SushiTrack, each in its own
`docs/agent/2026_10_26_PROFILER_AND_LOG_ADOPTION/SPEC.md`) found that v0.1.0 cannot serve a library
that is itself a shared library, nor a source whose data arrives after `begin()`. This release
closes the gaps that block SushiRuntime's device source, which every later phase (P3 to P6) and
SushiAI's inference profile wait on. The other gaps they found go to `docs/design/REMAINING_WORK.md`.

## Decisions

The owner delegated these to the orchestrator on 2026-10-26, asking for the SOLID choice that
holds as the stack grows.

| Gap (runtime / DSP / track id) | Decision | Why this one |
| --- | --- | --- |
| One collector per binary image (G1 / SPG-6 / S1) | Ship `SushiProfiler::SushiProfiler` as a shared library with hidden visibility and `SUSHIPROFILER_API` exports, and `SushiProfiler::SushiProfilerStatic` as a static library for a plug-in that must not share a host's collector, exactly as SushiLog ships | A process has one collector only if every image reaches the same one; a function table (an indirect call per zone) or a collector per library (a host sees no library zone) each break a promise of the design |
| A source cannot intern after `begin()` (G3) | The `SourceContext&` that `begin()` receives stays valid until `end()` returns, and `collect()` may call `intern`, `add_track` and `to_record_ns` on it. `begin`, `collect` and `end` all run on the consumer thread, so no lock is added | It states a contract the code already keeps instead of adding a second path to the name table |
| A source cannot learn which period its work falls in (G4) | `Source::collect` receives a `PeriodSpan` (`index`, `start_steady_ns`, `end_steady_ns`) in place of the bare index. A source places a piece of work in the period whose span holds its start time, and drops work that starts before the span it is given | The source needs no handle to a timeline and no clock of its own; the period is decided by time, which the source already has |
| Work that arrives after its period closed is lost silently (G6) | `SpanSink::drop_late(count)` adds to the period's dropped-span count, so the record says how much is missing; `consumer.linger_ms` remains the knob that makes it rarer | A record that is incomplete says so; a longer wait or a reopened period would cost every client for an asynchronous one |
| No registration pattern (G8) | A library exports `[[nodiscard]] <Lib>ProfilerParts register_profiler_parts(SushiProfiler::Registry&, ...)`, returning an object that owns its source and contributor and outlives the sessions that use them. A guide page states it and a test registers a fake library through it | One shape for every library, and ownership the type system shows |
| `device_idle` cannot find the host zone open during a gap (RW-4) | Keep the zone runs of active periods in the record, which `fold_period` already produces, and attribute each gap to the innermost zone open on the submitting thread. The record format's version rises | The attribution is why the profiler exists (a GPU idle 54 to 80 % of a training step); a record keeps raw data and the analysis can be rerun |
| A span cannot say which graph or which caller-level operation it came from (found writing SushiAI's plan) | `DeviceSpanKeys` gains two name attributes, `K_GRAPH = "graph"` (the label of the graph whose plan ran the node) and `K_ORIGIN = "origin"` (the caller's label for the operation the node was recorded for, such as an IR node). `roofline` groups by graph and origin as well, and writes them as columns; a span without them groups as before | A node index is unique only within one plan, so two graphs on one runtime merge into one row without the graph; the origin lets a library above say which of its operations a kernel belongs to without a second table |
| The idle drainer's budget (RW-5) | The budget row reads "At most one wake-up a second" | The measured bound is the design's bound; a longer slowest poll would delay the first drain of a burst |

Not in this release, because no inference-path client needs them: a per-period fact from a source
(G2; SushiRuntime writes its execution layer as a span attribute instead), knowing ahead whether a
period will be recorded (G5; SushiRuntime keeps device timing on for the whole session), and every
DSP and SushiTrack gap. Each has its entry in `docs/design/REMAINING_WORK.md`.

## The version

Changing `Source::collect` and the record format breaks the public interface, so the release is
v0.2.0 with `!` on the breaking commits. Nothing outside this repository implements `Source` yet.
