# tests/capture/zones

- `recording_fixture.hpp`: `RecordingFixture` opens the process-wide collector before each test,
  closes and empties it after, and `drain_mine()` returns what the test's thread wrote.
  `OwnedRecordingFixture` gives each test a `Collector` of its own on a `ManualTickSource`, opened,
  with the same `drain_mine()`; the small tests use it.
- `allocation_counter.hpp`, `allocation_counter.cpp`: replace the program's global `operator new`
  and `delete` with `malloc` and `free` that count each thread's allocations;
  `allocations_on_this_thread()` reads the count.
- `benchmark_zones.cpp`: the program `sp_bench_zones`, built with `sp build --benchmarks --type release` and run
  by `sp bench`. It prints the median nanoseconds of 31 runs of 1,000,000 iterations, after 3
  warm-up runs, for a zone compiled out, with the gate closed, at a disabled site and recording,
  a counter recording, and one read of each clock.
- `test_admission.cpp`: on a collector of its own, an enabled site gets the thread's ring and an
  id; a closed collector and a site the filter disables get none.
- `test_zone.cpp`: a zone writes one complete event with its site's name and line; nested zones
  nest by time; a named zone carries its name; a disabled site, a zone that outlives the
  collector's close and a compiled-out zone write nothing; compiled-out zones and lanes are
  empty types. A zone given a collector the test owns records into it with the stamps of its
  `ManualTickSource` and writes nothing while that collector is closed; these tests are small and
  reach no process-wide collector.
- `test_records.cpp`: on a collector of its own, a counter keeps its value, operation and unit; a
  value keeps its sample; an external zone keeps the caller's times past 2^32; an external
  instant keeps its time; each flow kind carries its item; a lane brackets the events inside it.
  One medium test checks that compiled-out record macros write nothing.
- `test_macros.cpp`: medium tests, one per direct macro, each checking the kinds that macro writes
  at the default level 2; after each site's first use, 100,000 rounds of zones, counters, values,
  instants, flows and lanes allocate nothing on the recording thread.
- `test_named_records.cpp`: on a collector of its own, counters and values named at run time
  become one site per name with their operation and unit, a value keeps every sample, and a
  counter and a value of one name are two sites. Two medium tests check the macros: the name
  expression is not evaluated with the gate closed, and `SP_THREAD_NAME` names the ring.
- `test_named_macros.cpp`: compiled with `SP_PROFILE_LEVEL` 0, `SP_THREAD_NAME` and the named
  macros do not evaluate their argument or name (small tests, which reach no collector), and
  `SP_THREAD_NAME` names nothing and the named macros write nothing (medium tests, which reach
  the process-wide collector).
