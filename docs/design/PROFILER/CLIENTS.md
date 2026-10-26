# The libraries that use SushiProfiler

## SushiRuntime

- `sushiruntime.device`: a source that converts each run's `node_events` through `ClockPairing`
  into spans, with `node`, `invocation`, `operation` and the widths as attributes, and reports the
  run's execution layer for the period.
- `NodeEventRecord` carries no operation id today. P3 adds to `RunReport` a node table, one row
  per registered node with its name and `OperationID`, indexed by `node_index`. The change only
  adds a field; no existing member moves.
- SushiProfiler is an optional dependency of each library: an option (`SR_ENABLE_PROFILER`, and
  likewise in SushiBLAS and SushiAI) defaults to on; off, the zone macros expand to nothing and the
  sources are not built, and the library builds without SushiProfiler present.
- `SR_ZONE` in plan build, submit, wait and poll, under the group `Runtime`.
- The graph asks for node events only while the hub wants device spans.

## SushiBLAS

- A cost rule per operation id, giving FLOPs and bytes from the operands' shapes and types,
  written as span attributes. A test proves every recorded operation has a rule or is declared
  uncosted, as SushiAI's autograd table proves backward rules.
- `device_ceilings`, a benchmark `sb bench` runs: FMA loops per precision, a streaming copy, an
  empty kernel. It writes `<device>.ceilings.json`.
- `roofline`, the analysis in [RECORD_AND_ANALYSES.md](RECORD_AND_ANALYSES.md).
- The benchmark harness marks iterations on the timeline `bench.iteration`, so every benchmark
  writes a record.

## SushiAI

- `SA_ZONE` groups: `Data` (read, decode, augment), `Host` (feed, assignment, loss read, measure),
  `Copy` (input copy), `Device` (the step's runs and waits), `Optimizer`, `Evaluation` (decode,
  NMS, mean average precision).
- The timelines `train.step` and `inference.batch`.
- `sushiai.attribution`: a source that adds `ir_node` and `op` to each span. Lowering records,
  per IR node, the range of runtime registration indices it emitted (`first`, `count`), kept by
  the compiled step. `NodeEventRecord::node_index` is the registration index, so the join is a
  lookup. A test lowers a graph whose node emits three tasks and checks all three map back to it.
- Two runs answer the owner's question together. A run with `level` 1 and runtime profiling off
  measures the true step time and its host groups on the production execution path. A run with
  runtime profiling on gives the kernel breakdown on the node-by-node path. `sp compare` of the two
  states the cost of the second, so its findings are read with that cost in view.
- A `profile` section in the training and inference run files holding a `ProfilerConfig`. Absent,
  nothing records and the gate never opens.
- `sa profile show` and `sa profile compare`, forwarding to `sp`.
