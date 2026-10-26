# tests/capture/zones

- `recording_fixture.hpp`: `RecordingFixture` opens the collector before each test, closes and
  empties it after, and `drain_mine()` returns what the test's thread wrote.
- `allocation_counter.hpp`, `allocation_counter.cpp`: replace the program's global `operator new`
  and `delete` with `malloc` and `free` that count each thread's allocations;
  `allocations_on_this_thread()` reads the count.
- `benchmark_zones.cpp`: the program `sp_bench_zones`, built with `sp build --benchmarks` and run
  by `sp bench`. It prints the median nanoseconds of 31 runs of 1,000,000 iterations, after 3
  warm-up runs, for a zone compiled out, with the gate closed, at a disabled site and recording,
  a counter recording, and one read of each clock.
- `test_admission.cpp`: an enabled site gets the thread's ring; a closed collector and a site the
  filter disables get none.
- `test_zone.cpp`: a zone writes one complete event with its site's name and line; nested zones
  nest by time; a named zone carries its name; a disabled site, a zone that outlives the
  collector's close and a compiled-out zone write nothing; compiled-out zones and lanes are
  empty types.
- `test_records.cpp`: a counter keeps its operation, unit and value; a value keeps its sample;
  an external zone keeps the caller's times past 2^32; an external instant keeps its time; flows
  carry their item; a lane brackets the events inside it; compiled-out records write nothing.
- `test_macros.cpp`: every direct macro writes its kind at the default level 2; after each site's
  first use, 100,000 rounds of zones, counters, values, instants, flows and lanes allocate nothing
  on the recording thread.
