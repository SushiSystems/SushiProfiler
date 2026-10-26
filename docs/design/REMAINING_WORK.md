# Remaining work

Next id: RW-5

## Next

- RW-2 Every later phase reads the record, and the libraries above wait on its release

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

## Session and record

### RW-2 Build the session, the run record and the `sp` tools
- State: open
- Done when: a test program's periods, counters, values, instants and flows give a record, a
  terminal summary and a Chrome trace; two records compare; a check fails when it should; a record
  round-trips and validates
- Source: `docs/design/PROFILER/README.md`

## Later

### RW-3 Add a C ABI, a Python record writer and the later analyses
- State: open
- Done when: each of the P9 items has its own design and is built or withdrawn
- Source: `docs/design/PROFILER/README.md`
