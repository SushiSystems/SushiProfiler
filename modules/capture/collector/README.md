# collector

`SushiProfiler/collector/collector.hpp` declares `Collector`, the one process-wide object of
SushiProfiler, in the
namespace `SushiProfiler`. It depends on `clock`, `events`, `sites` and `rings`. A recording macro
at any call site must reach it, so it is the one singleton; it is built on first use and never
destroyed, so a thread that exits after `main` returns still retires into it.

- The gate. `Collector::recording()` is one relaxed load of a 32-bit atomic.
  `open(CollectorSettings)` applies the site filter, discards what the rings held from before, frees
  retired rings, takes the first calibration point and opens the gate; a second `open` returns
  false. `close()` closes the gate and leaves the rings for one more drain.
- The clock. `Collector::now()` stamps events with TSC ticks when the counter is invariant and with
  steady nanoseconds otherwise; the collector chooses once, and `DrainBatch::ticks_are_tsc` says
  which.
- The rings. `Collector::ring()` returns the calling thread's ring, registering it on first use
  while the gate is open. A thread that must not allocate on its first record, such as a real-time
  audio thread, calls `register_this_thread(capacity)` beforehand, open or closed. Threads get
  indices from 0 that are never reused. `set_thread_name` names the ring.
- The drain. `drain(DrainBatch&)` takes a calibration point and moves each ring's events, drops and
  fill into one `ThreadDrain`; a thread that has exited is drained once more with `retired` set,
  then its ring is freed. One consumer drains. The collector has no condition variable and never
  signals: the consumer polls at an interval it chooses from the fill, so a producer never makes a
  system call.
- `sites()`, `names()` and `apply(SiteFilter)` give the process's site registry and name table.

Two `thread_local` guards retire a ring when its thread exits: one built at registration, and a
namespace-scope backstop that the MSVC ABI builds at thread attach and destroys last, since that
ABI never runs the destructor of a `thread_local` built during TLS teardown. A thread that first
records from a `thread_local` destructor is therefore retired on both ABIs, and a thread whose
guard has run gets no new ring.

`CollectorSettings` (`collector_settings.hpp`) holds the ring size and the filter; `ThreadDrain` and
`DrainBatch` are in `drain_batch.hpp`. The tests are in `tests/capture/collector/`.
