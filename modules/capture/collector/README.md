# collector

`SushiProfiler/collector/collector.hpp` declares `Collector`, in the namespace `SushiProfiler`. It
depends on `clock`, `events`, `sites` and `rings`. A collector holds one gate, its sites, its names
and one ring per thread that records into it.

- Owned or process-wide. `Collector(const TickSource&)` builds a closed collector that stamps
  events with the tick source it is given; its owner passes it to what records into it: a zone,
  a record function, a timeline, a window or a session. Its site registry keeps the ids it gives
  in a map (`SiteCache::InRegistry`), so the site handles stay the process-wide collector's.
  `Collector::instance()` is the process-wide one, which the recording macros reach: it stamps with
  a `TraceTickSource`, caches site ids in the handles, is built on first use and is never
  destroyed, so a thread that exits after `main` returns still retires into it.
- The gate. `is_recording()` is one relaxed load of a 32-bit atomic. `open(CollectorSettings)`
  applies the site filter, discards what the rings held from before, frees retired rings, takes
  the first calibration point and opens the gate; a second `open` returns false. `close()` closes
  the gate and leaves the rings for one more drain.
- The session and the gate. `open` starts a session and `close` ends it; `is_open` says whether
  one runs. Two reasons close the gate inside it, and the gate is open only while neither holds.
  The window's reason is `pause` and `resume`: `pause` closes the zone gate and the period marks,
  `resume` opens them again, without discarding what the rings hold, so a capture window can skip
  periods. The caller's reason is `hold` and `release`: `hold` closes the zone gate but leaves
  `is_marking()` true, so period marks are still written and a paused run keeps its period
  boundaries; the window's next `resume` cannot undo a hold. `resume`, `hold` and `release` do
  nothing while no session is open, and `close` clears a hold. `is_marking()` is one relaxed load,
  and `thread_mark_ring()` is `thread_ring()` for a period mark: it registers the calling thread
  while marks are written, even when the gate is held.
- Sources. `wants_sources()` is one relaxed load that a source reads to pay for device timing only
  while a session wants it. The session sets it with `set_sources_wanted`: true after its sources
  began, false while the session is paused, true again on resume, and false before its sources
  end.
- The clock. `stamp()` reads the tick source: TSC ticks when the counter is invariant and steady
  nanoseconds otherwise for `TraceTickSource`, the time it was set to for `ManualTickSource`.
  `DrainBatch::ticks_are_tsc` says which.
- The rings. `thread_ring()` returns the calling thread's ring, registering it on first use while
  the gate is open. A thread that must not allocate on its first record, such as a real-time audio
  thread, calls `register_thread(capacity)` beforehand, open or closed. Threads get indices from 0
  that are never reused. `name_thread` names the ring. A thread caches the ring of the last
  collector it reached, keyed by a serial no other collector of the process carries, so a thread
  that records into one collector pays one compare per record; reaching another collector looks
  the ring up under that collector's lock.
- The drain. `drain(DrainBatch&)` takes a calibration point and moves each ring's events, drops and
  fill into one `ThreadDrain`; a thread that has exited is drained once more with `retired` set,
  then its ring is freed. One consumer drains. The collector has no condition variable and never
  signals: the consumer polls at an interval it chooses from the fill, so a producer never makes a
  system call.
- `sites()`, `names()` and `apply(SiteFilter)` give the collector's site registry and name table.

The statics `recording()`, `now()`, `marking()`, `sources_wanted()`, `want_sources()`, `ring()`,
`mark_ring()`, `register_this_thread()`, `set_thread_name()` and `thread_index()` are the same
calls on the process-wide collector, for the macros and the callers that predate passing one in.
`recording()` costs two loads while the process-wide collector is closed, and false before it is
built.

The library is shared, so the process-wide `Collector` lives in one image and every image of a
process that records through its macros reaches that one object. The static target
`sushiprofiler_static` is linked into one image and keeps a process-wide collector of its own.

Two `thread_local` guards retire a thread's rings in every live collector when the thread exits:
one built at registration, and a namespace-scope backstop that the MSVC ABI builds at thread attach
and destroys last, since that ABI never runs the destructor of a `thread_local` built during TLS
teardown. A thread that first records from a `thread_local` destructor is therefore retired on both
ABIs, and a thread whose guard has run gets no new ring. A collector leaves the list of live
collectors when it is destroyed.

`CollectorSettings` (`collector_settings.hpp`) holds the ring size and the filter; `ThreadDrain` and
`DrainBatch` are in `drain_batch.hpp`. The tests are in `tests/capture/collector/`.
