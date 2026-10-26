# What the stack measured on 2026-10-26

## SushiRuntime

- `RuntimeConfig::profiling` (default false) builds the SYCL queues with `enable_profiling`.
- `RunReport` carries `node_events` (submit, start and end on the event clock, launched and live
  width per dispatch), `ClockPairing` to `steady_clock`, the `ExecutionLayer` that ran, and
  host-to-device and device-to-host bytes.
- With profiling on, a chainable plan runs node by node on one in-order queue; without it, it may
  replay one `command_graph` per step.

## SushiBLAS

No timing outside `benchmark_harness` (ten iterations, a printed median). No FLOP or byte count.
Every kernel is recorded through `TaskRecorder::insert` with a task name and an operation id.

## SushiAI

- `sa train` builds a runtime with profiling off, and the training program calls `run_step`
  directly, so `Trainer::fit`'s per-node sum never runs.
- No host phase is timed.
- The anchor-free step runs the forward part on the device, reads ports back, assigns targets on
  the host, writes the loss leaves and runs the rest. That pause can explain low utilisation as
  well as slow kernels can.
- Lowering keeps one running total of the tasks it emitted (`LoweringContext::count`), not a
  count per IR node.

## SushiEngine

`modules/foundation/profiling`, about 4,700 lines: an invariant-TSC clock calibrated to
`steady_clock`; a single-producer ring of 24-byte events per thread; a recorder that drains the
rings every 5 ms; zones with groups and compile-time levels; device tracks; a live consumer beside
the file capture; a Chrome trace writer; `FrameSampler` and `ZoneTreeFolder`; an optional Tracy
backend; a test that the zone thread allocates nothing. `execution/run_report_trace.hpp` turns a
SushiRuntime `RunReport` into device spans.

## SushiDSP

A real-time audio library on the CPU, depending on SushiLog alone. `AudioNode::process()` runs on
the audio thread and may not allocate, lock, call the kernel or throw. Its `benchmark_harness`
times 20 warm-up and 200 timed repetitions and keeps the minimum, which hides the slow blocks that
cause dropouts; `solver_counters.hpp` counts solves, failed solves and the worst iteration count;
four room and GUI tests time code by hand against millisecond bounds; `timing_gate.hpp` skips a
bound in an unoptimised build. Gates are fractions of a 256-frame block (2.67 ms, 2.0 ms, 0.15 ms
in its design documents). No load meter, dropout count or jitter is measured.

## SushiTrack

A C++17 multi-object tracker called once per frame, with a C ABI, a Python binding, a Python
inference loop and a benchmark runner; it depends on SushiLog alone. The one C++ timing sums
`steady_clock` microseconds around `update` and prints a mean (`mot_runner.cpp:67-71`); the Python
loop times with `time.time()`; recorded results carry no date, commit or machine. Its backlog
items RW-11 and RW-12 cannot close without a per-frame cost to compare. Per-frame counts (objects
in and out, matches per stage) are measured nowhere.
