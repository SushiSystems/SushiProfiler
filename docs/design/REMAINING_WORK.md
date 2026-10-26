# Remaining work

Next id: RW-23

## Session

### RW-7 Let a source state a fact of one period
- State: open
- Done when: `SpanSink` sets a named fact on the period it fills and the record writes it

### RW-8 Tell a source before a period opens whether it will be recorded
- State: open
- Done when: a source learns the window's next recorded period ahead of its mark

### RW-20 State and test a second start after a stop in one process
- State: open
- Done when: a test starts, stops and starts a session again and reads two records

### RW-21 Add run tags to the configuration
- State: open
- Done when: `ProfilerConfig` holds tags and the record's `tags` table carries them

### RW-22 Drain a burst before a ring fills
- State: open
- Done when: a burst after a quiet second loses no event on a ring sized by the configuration

### RW-6 Apply `buffers.attribute_bytes` or remove it
- State: open
- Done when: a period's span attributes stop at the configured byte count and the excess is
  counted as dropped spans, or the key leaves the configuration format. The reader accepts and
  range-checks it today, and no part uses it
- Source: `docs/design/PROFILER/CAPTURE_AND_SESSION.md`

## Capture

### RW-10 Register a site before its first record
- State: open
- Done when: a program registers its sites at start and the first record on a real-time thread
  takes no lock and allocates nothing

### RW-11 Measure a callback's load apart from its arrival interval
- State: open
- Done when: a period driven by a device callback reports the time the callback was busy beside
  the time between marks

### RW-12 Add an aggregated zone that writes one event per period
- State: open
- Done when: a site sums its ticks and calls over a period and writes one event when it closes

### RW-13 Name a counter or a value at run time
- State: open
- Done when: `SP_COUNTER_AT` and the value macro take a name built at run time, as
  `SP_ZONE_NAMED_AT` does

### RW-16 Add a scoped period
- State: open
- Done when: an object marks a period at construction and closes it at destruction

## Record

### RW-9 Show overlapping spans on one track as they ran
- State: open
- Done when: a Chrome trace of two spans that overlap in part on one track draws both whole

### RW-14 Read the last closed period's statistics while a session runs
- State: open
- Done when: a program reads a closed period's statistics without stopping the session

### RW-15 Add counts over budget, mean, sum and a baseline to checks
- State: open
- Done when: a check reads periods over budget, the mean and the sum, compares with a stored
  record and is skipped on a build fact

### RW-17 Add the date, the commit and the host name to the facts
- State: open
- Done when: a record carries the date and the host name, and a library contributor adds its commit

### RW-18 Write a period table a program can read
- State: open
- Done when: a writer emits each period's statistics as CSV or JSON a script parses

### RW-19 Record every period of a run of unknown length
- State: open
- Done when: the window takes "every period" without a count

## Later

### RW-3 Add a C ABI, a Python record writer and the later analyses
- State: open
- Done when: each of the P9 items has its own design and is built or withdrawn
- Source: `docs/design/PROFILER/README.md`
