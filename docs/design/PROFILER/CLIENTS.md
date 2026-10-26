# The libraries that use SushiProfiler

## Device span keys

Every device source writes the same attributes and facts, so a record from SushiRuntime's
`sushiruntime.device` and one from SushiEngine's `sushiengine.native` read the same way. The
constants are in `SushiProfiler/record/device_span_keys.hpp`, in `SushiProfiler::DeviceSpanKeys`;
a source and an analysis use them and never write the word again.

| Key | Kind | Meaning | Constant |
| --- | --- | --- | --- |
| `node` | integer span attribute | The index of the graph node that ran | `K_NODE` |
| `graph` | name span attribute | The label of the graph whose plan ran the node; a node index is unique only within one plan, so `roofline` keeps graphs apart | `K_GRAPH` |
| `origin` | name span attribute | The caller's label for the operation the node was recorded for, such as an IR node; `roofline` keeps origins apart | `K_ORIGIN` |
| `invocation` | integer span attribute | Which invocation of the graph the span belongs to | `K_INVOCATION` |
| `operation` | integer span attribute | The id of the operation the node performs | `K_OPERATION` |
| `launched_width` | integer span attribute | The work items the launch started | `K_LAUNCHED_WIDTH` |
| `live_width` | integer span attribute | The work items that did useful work | `K_LIVE_WIDTH` |
| `dtype` | name span attribute | The element type the span computed in, such as `f32`; selects the compute ceiling | `K_DTYPE` |
| `flops` | integer span attribute | The floating-point operations the span performed; read by `roofline` | `K_FLOPS` |
| `bytes` | integer span attribute | The bytes the span moved to and from memory; read by `roofline` | `K_BYTES` |
| `device.<track>.peak_flops` | environment fact, a number as text | The track's compute ceiling in FLOP/s, for spans without a type or without a ceiling of their own; read by `roofline` | `peak_flops_fact(track)` |
| `device.<track>.peak_flops.<dtype>` | environment fact, a number as text | The track's compute ceiling in FLOP/s for one element type; read by `roofline` before the type-less fact | `peak_flops_fact(track, dtype)` |
| `device.<track>.peak_bytes_per_s` | environment fact, a number as text | The track's memory ceiling in bytes per second; read by `roofline` | `peak_bytes_per_s_fact(track)` |

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
- The graph asks for node events only while the collector wants device spans.

## SushiBLAS

- A cost rule per operation id, giving FLOPs and bytes from the operands' shapes and types,
  written as span attributes. A test proves every recorded operation has a rule or is declared
  uncosted, as SushiAI's autograd table proves backward rules.
- `device_ceilings`, a benchmark `sb bench` runs: FMA loops per precision, a streaming copy, an
  empty kernel. It writes `<device>.ceilings.json`.
- The ceilings as environment facts, `device.<track>.peak_flops` and
  `device.<track>.peak_bytes_per_s`, read from `<device>.ceilings.json` by a contributor. With the
  cost attributes, they are all `roofline` needs; the analysis is SushiProfiler's generic one in
  [RECORD_AND_ANALYSES.md](RECORD_AND_ANALYSES.md).
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
  runtime profiling on gives the kernel breakdown on the node-by-node path. `sp record compare` of the two
  states the cost of the second, so its findings are read with that cost in view.
- A `profile` section in the training and inference run files holding a `ProfilerConfig`. Absent,
  nothing records and the gate never opens.
- `sa profile show` and `sa profile compare`, forwarding to `sp`.
