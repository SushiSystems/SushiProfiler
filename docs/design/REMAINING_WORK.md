# Remaining work

Next id: RW-7

## Decisions waiting for the owner

### RW-4 Decide where `device_idle` finds the host zone open during a device gap
- State: decision
- Done when: a gap is attributed to the host zone open on the submitting thread, as
  `RECORD_AND_ANALYSES.md` says, or the design drops that attribution. The record holds zone
  trees, not runs, so the analysis splits gaps into `launch` and `host` only. The choices are to
  keep the zone runs of active periods in the record, which `fold_period` already produces, or to
  attribute gaps in the session when a period closes and store the result per period. Both change
  the record format
- Source: `docs/design/PROFILER/RECORD_AND_ANALYSES.md`

### RW-5 Decide whether the idle drainer's budget reads "at most one wake-up a second"
- State: decision
- Done when: the design's budget row and the consumer's slowest poll agree. An idle consumer
  measured 1.000 wake-ups a second with `sp_bench_consumer`, the bound itself, against a budget of
  "under one". A slowest poll above 1 s meets the row as written, and makes a burst after a quiet
  stretch wait longer for its first drain
- Source: `docs/design/PROFILER/README.md`

## Session

### RW-6 Apply `buffers.attribute_bytes` or remove it
- State: open
- Done when: a period's span attributes stop at the configured byte count and the excess is
  counted as dropped spans, or the key leaves the configuration format. The reader accepts and
  range-checks it today, and no part uses it
- Source: `docs/design/PROFILER/CAPTURE_AND_SESSION.md`

## Later

### RW-3 Add a C ABI, a Python record writer and the later analyses
- State: open
- Done when: each of the P9 items has its own design and is built or withdrawn
- Source: `docs/design/PROFILER/README.md`
