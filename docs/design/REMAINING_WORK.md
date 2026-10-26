# Remaining work

Next id: RW-4

## Next

- RW-1 Every other phase builds on the capture layer
- RW-2 The libraries above wait on its release

## Capture

### RW-1 Build the capture layer of the profiler
- State: open
- Done when: the zone, ring and allocation tests pass and the budget benchmarks meet the budget of
  the design document
- Source: `docs/design/PROFILER/README.md`

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
