# The capture layer: implementation plan

**Status:** Open — written 2026-10-26; executed inline by the orchestrator, as the owner chose.
**Implements:** PROFILER#P1

> For agentic workers: REQUIRED SUB-SKILL: superpowers:executing-plans. Steps use checkbox
> (`- [ ]`) syntax for tracking.

**Goal:** The foundation and capture tiers of SushiProfiler: a clock, the event types, site and
name registries, per-thread rings, the process-wide collector, zones and the other event kinds, a lock
that records its waits, period timelines, and the benchmarks that prove the budget.

**Architecture:** Eight modules in two tiers, composed into the one static library. The hot path
is inline: a gate load, a site enable byte, two clock reads and one 24-byte ring store. The collector is
the only process-wide state. Draining and folding belong to P2's consumer; P1 exposes
`Collector::drain` and tests read it directly.

**Tech stack:** C++17, CMake through `sp`, GoogleTest, MSVC 2022 and clang.

**Spec:** `docs/design/PROFILER/README.md` and its pages, chiefly `CAPTURE_AND_SESSION.md` and
`ENGINE_CHANGES.md`. The engine source being moved: `D:/Projects/sushiengine/modules/foundation/profiling`
(read only; another session works there).

## Global constraints

- Dependencies: the C++17 standard library and Threads only. No logging; failures return values.
- Namespace `SushiProfiler`; internals in `SushiProfiler::Detail`. Headers under
  `modules/<tier>/<module>/include/SushiProfiler/<module>/`, sources under `source/`.
- Tests mirror sources: `tests/<tier>/<module>/test_<file>.cpp`; suites open with `Small_` (one
  thread, no sleep) or `Medium_` (threads).
- Every file opens with the licence block from `tools/licensing/write_license_block.py`
  (`--project 'SushiProfiler - https://github.com/sushisystems/sushiprofiler'`), then the `@file`
  block; every symbol carries a Doxygen block (`source-comments`).
- Allman braces, 4 spaces, 100 columns, `K_UPPER` constants, `snake_case_` members (`cpp-code-style`).
- The producer path never allocates (after `register_this_thread`), never locks, never makes a
  system call, never throws, never blocks.
- Builds and tests through `sp` only, 4 jobs (`CMAKE_BUILD_PARALLEL_LEVEL=4`); never cancel a build.
- After each task: `python -I tools/check_all.py .` clean, then one commit.

## Review focus

1. A thread that exits while the collector is open: its ring's events are still drained once, then the
   ring is freed, with no crash at process exit (Task 5, `Medium_Collector.AnExitedThreadIsDrainedThenFreed`).
2. A full ring: the push fails without blocking and the drop is counted and reported by the next
   drain (Task 4, `Small_ThreadRing.AFullRingDropsAndCounts`; Task 5, `Small_Collector.DropsReachTheDrain`).
3. A zone opened while the collector is open and closed after `close()`: nothing is written, nothing
   crashes (Task 6, `Small_Zone.AZoneClosedAfterTheCollectorClosesWritesNothing`).
4. More sites than the registry holds: the extra site gets id 0, is never recorded, and is counted
   (Task 3, `Small_SiteRegistry.ASiteBeyondCapacityIsCountedNotRecorded`).
5. Values that are not ordinary: a NaN counter value and a zone longer than 4.29 s survive the
   event round trip unchanged (Task 2, `Small_TraceEvent.ANaNValueKeepsItsBits`,
   `Small_TraceEvent.ASpanLongerThanThirtyTwoBitsSurvives`).

---

### Task 0: First commits of the skeleton

- [ ] Commit SushiProfiler's skeleton (with the two CMake fixes and the design topic) as
      `build: create the repository skeleton` and `docs(design): open the profiler topic`.
- [ ] Commit SushiStack's catalog change: `feat(hub): add sushiprofiler to the catalog`.
- [ ] Commit SushiAI's SPEC: `docs(agent): design the stack profiler`.
- [ ] Run `write_license_block.py --report` in SushiProfiler: expect `0 files pending`.

### Task 1: `clock` (foundation)

**Files:** create `modules/foundation/clock/{CMakeLists.txt,README.md}`,
`include/SushiProfiler/clock/{trace_clock.hpp,calibration.hpp}`, `source/trace_clock.cpp`; tests
`tests/foundation/clock/{CMakeLists.txt,README.md,test_trace_clock.cpp,test_calibration.cpp}`;
modify `cmake/SushiProfilerTiers.cmake` (add `clock foundation` before `umbrella`).

**Interfaces (produces):**
- `struct CalibrationPoint { std::uint64_t ticks; std::uint64_t steady_ns; };`
- `class TraceClock` (static only): `static std::uint64_t now() noexcept` (TSC through
  `__rdtsc` on MSVC and `__builtin_ia32_rdtsc` on GCC/clang, x86 only; `steady_now_ns()`
  elsewhere), `static std::uint64_t steady_now_ns() noexcept`, `static bool ticks_are_invariant() noexcept`
  (CPUID leaf 0x80000007 EDX bit 8; cached), `static CalibrationPoint sample() noexcept` (tightest
  of 16 brackets, as the engine).
- `class Calibration`: `Calibration() noexcept`, `Calibration(const CalibrationPoint& begin, const CalibrationPoint& end) noexcept`,
  `std::uint64_t to_nanoseconds(std::uint64_t ticks) const noexcept`, `double nanoseconds_per_tick() const noexcept`.
- When ticks are not invariant, callers use `steady_now_ns()` as ticks and an identity calibration;
  the collector (Task 5) makes that choice once.

- [ ] Tests: `Small_TraceClock.SteadyTimeNeverGoesBack` (1,000 reads non-decreasing);
      `Small_TraceClock.SampleBracketsTheSteadyReading` (a sample's `steady_ns` lies between two
      steady reads taken around it); `Small_Calibration.MapsBothEndsExactly` (points (1000, 5000)
      and (3000, 6000): 1000 → 5000, 3000 → 6000, 2000 → 5500); `Small_Calibration.ExtrapolatesBeforeTheBegin`
      (500 → 4750); `Small_Calibration.EqualTicksKeepOneNanosecondPerTick`.
- [ ] Build and test: `sp build`, `sp test` (see Task 0's environment); expect the new tests fail
      to link before `trace_clock.cpp` exists, then pass.
- [ ] README of the module; commit `feat(clock): read the TSC and calibrate it to steady time`.

### Task 2: `events` (foundation)

**Files:** `modules/foundation/events/` with `include/SushiProfiler/events/{ids.hpp,trace_event.hpp}`,
header only; tests `tests/foundation/events/test_trace_event.cpp`; tier line `events foundation`.

**Interfaces (produces):**
- `using SiteID = std::uint32_t; using NameID = std::uint32_t;` (0 means none),
  `inline constexpr SiteID K_MAX_SITE_ID = (1U << 28) - 1U;`
- `enum class EventKind : std::uint8_t { Zone, ExternalZone, Instant, ExternalInstant, Counter, Value, PeriodMark, PeriodTag, FlowBegin, FlowStep, FlowEnd, LaneBegin, LaneEnd };`
  (13 kinds; the top 4 bits of the site word.)
- `struct TraceEvent { std::uint64_t begin; std::uint64_t end; std::uint32_t site_word; NameID name; };`
  `static_assert(sizeof(TraceEvent) == 24)` and trivially copyable.
- `constexpr std::uint32_t pack_site(EventKind, SiteID) noexcept`, `constexpr EventKind kind_of(const TraceEvent&) noexcept`,
  `constexpr SiteID site_of(const TraceEvent&) noexcept`.
- `std::uint64_t value_bits(double) noexcept` and `double value_of(std::uint64_t) noexcept` through
  `std::memcpy`.
- Field meaning per kind, written in the header's table: Zone and ExternalZone: begin and end
  (ticks; steady ns for External); Instant: begin = end = time; Counter and Value: begin = time,
  end = value bits; PeriodMark: begin = time, end = budget ns or 0, name = timeline;
  PeriodTag: begin = time, end = `(key << 32) | value` interned names, name = timeline;
  Flow*: begin = time, end = the caller's id; Lane*: begin = time, name = lane.

- [ ] Tests: `Small_TraceEvent.IsTwentyFourBytes`; `Small_TraceEvent.KindAndSiteRoundTrip` (every
      kind with sites 1 and `K_MAX_SITE_ID`); `Small_TraceEvent.ANaNValueKeepsItsBits` (quiet NaN
      with payload 0x7ff8000000001234 round-trips bit for bit; also +inf and -0.0);
      `Small_TraceEvent.ASpanLongerThanThirtyTwoBitsSurvives` (begin 0, end 5'000'000'000).
- [ ] Build, test, README, commit `feat(events): define the 24-byte trace event and its kinds`.

### Task 3: `sites` (capture)

**Files:** `modules/capture/sites/` with `include/SushiProfiler/sites/{site.hpp,site_registry.hpp,name_table.hpp}`,
`source/{site_registry.cpp,name_table.cpp}`; tests `tests/capture/sites/{test_site_registry.cpp,test_name_table.cpp}`;
tier lines `sites capture`; depends on `events`.

**Interfaces (produces):**
- `enum class CounterOp : std::uint8_t { Add, Set, Max, Min };`
  `enum class SiteRole : std::uint8_t { Zone, LockWait, Instant, Counter, Value, Flow, Timeline, Lane };`
- `struct Site { const char* name; const char* group; const char* file; std::uint32_t line; std::uint8_t level; SiteRole role; CounterOp op; const char* unit; };`
  (aggregate, built `static constexpr` by the macros; `group`, `unit` may be null.)
- `struct SiteHandle { const Site* site; std::atomic<SiteID> id{0}; };` (one static per call site).
- `struct SiteFilter { std::uint8_t level = 3; std::vector<std::string> groups; };` (empty groups keep all).
- `struct SiteInfo { std::string name, group, file, unit; std::uint32_t line; std::uint8_t level; SiteRole role; CounterOp op; };`
- `class SiteRegistry`: `explicit SiteRegistry(std::size_t capacity = K_DEFAULT_SITE_CAPACITY)` with
  `K_DEFAULT_SITE_CAPACITY = 65536`; `SiteID id_of(SiteHandle&)` (fast path: relaxed load of
  `handle.id`; slow path registers under a mutex, copies the strings, applies the filter);
  `bool enabled(SiteID) const noexcept` (one relaxed byte load from a preallocated array; id 0 is
  false); `void apply(const SiteFilter&)` (recomputes every byte); `SiteInfo info(SiteID) const`;
  `std::size_t size() const`; `std::uint64_t refused() const noexcept` (registrations past capacity).
- `class NameTable`: `NameID intern(std::string_view)` (same text, same id; 0 for empty text),
  `std::string text(NameID) const`, `std::size_t size() const`. Names are never freed.

- [ ] Tests: `Small_SiteRegistry.OneSiteGetsOneIdForEveryHandleCall`;
      `Small_SiteRegistry.TwoSitesWithEqualTextAreTwoSites` (identity is the handle, not the text);
      `Small_SiteRegistry.TheFilterDisablesALevelAboveIt` (level 2 site off under level 1);
      `Small_SiteRegistry.TheFilterKeepsOnlyNamedGroups`; `Small_SiteRegistry.AFilterAppliesToSitesRegisteredLater`;
      `Small_SiteRegistry.ASiteBeyondCapacityIsCountedNotRecorded` (capacity 2: third id is 0,
      `enabled(0)` false, `refused() == 1`); `Small_SiteRegistry.InfoCopiesTheStrings` (strings in a
      buffer that is then overwritten still read back); `Medium_SiteRegistry.ConcurrentFirstUseGivesOneId`
      (8 threads, one handle). `Small_NameTable.SameTextSameId`, `Small_NameTable.EmptyTextIsZero`.
- [ ] Build, test, README, commit `feat(sites): register sites once and filter them at run time`.

### Task 4: `rings` (capture)

**Files:** `modules/capture/rings/` with `include/SushiProfiler/rings/thread_ring.hpp`,
`source/thread_ring.cpp`; tests `tests/capture/rings/test_thread_ring.cpp`; tier `rings capture`;
depends on `events`.

**Interfaces (produces):**
- `class ThreadRing` (single producer, single consumer, the engine's layout): `explicit ThreadRing(std::size_t capacity_events)`
  (rounded up to a power of two, at least 64); `bool push(const TraceEvent&) noexcept` (inline;
  cached tail; on full counts a drop and returns false); `std::size_t drain(std::vector<TraceEvent>& out)`
  (appends, oldest first); `std::uint64_t take_dropped() noexcept`; `double fill() const noexcept`
  (occupied over capacity, read by the consumer to adapt its interval); `std::size_t capacity() const noexcept`;
  `void set_name(std::string_view) noexcept` and `std::string name() const` (the engine's seqlock
  of 64 atomic chars).

- [ ] Tests: `Small_ThreadRing.CapacityRoundsUpToAPowerOfTwo` (100 → 128, 1 → 64);
      `Small_ThreadRing.DrainsInOrder`; `Small_ThreadRing.AFullRingDropsAndCounts` (capacity 64:
      70 pushes, 64 kept, `take_dropped() == 6`, then 0); `Small_ThreadRing.FillFollowsOccupancy`
      (32 of 64 → 0.5); `Medium_ThreadRing.OneProducerOneConsumerLoseNothingWhenSized`
      (1,000,000 events through a 4096 ring with a draining thread: every event once, in order,
      or counted as dropped, sum equal to 1,000,000); `Small_ThreadRing.NameIsTruncatedAtSixtyThree`.
- [ ] Build, test, README, commit `feat(rings): keep each thread's events in a lock-free ring`.

### Task 5: `collector` (capture)

**Files:** `modules/capture/collector/` with `include/SushiProfiler/collector/{collector.hpp,collector_settings.hpp,drain_batch.hpp}`,
`source/collector.cpp`; tests `tests/capture/collector/test_collector.cpp`; tier `collector capture`; depends on `clock`,
`events`, `sites`, `rings`.

**Interfaces (produces):**
- `struct CollectorSettings { std::size_t ring_events = 65536; std::size_t site_capacity = 65536; SiteFilter filter; };`
- `struct ThreadDrain { std::uint32_t thread_index; std::string name; std::vector<TraceEvent> events; std::uint64_t dropped; double fill; bool retired; };`
- `struct DrainBatch { CalibrationPoint point; bool ticks_are_tsc; std::vector<ThreadDrain> threads; };`
- `class Collector` (the one process-wide object; never destroyed):
  `static Collector& instance()`;
  `static bool recording() noexcept` (the gate: `std::atomic<std::uint32_t>`, relaxed, non-zero is open);
  `static std::uint64_t now() noexcept` (TSC ticks when invariant, else steady ns, chosen at first use);
  `bool open(const CollectorSettings&)` (false when already open; discards stale ring contents; samples
  the first calibration point; opens the gate); `void close()` (closes the gate; keeps rings for one
  last drain); `bool is_open() const noexcept`;
  `void drain(DrainBatch& out)` (moves every ring's events, drops and fill into `out`, takes a
  calibration point, frees retired rings once drained);
  `SiteRegistry& sites() noexcept`; `NameTable& names() noexcept`; `void apply(const SiteFilter&)`;
  `static ThreadRing* ring() noexcept` (the calling thread's ring, registering it on first use
  while open; null after the thread retired or while closed and unregistered);
  `static bool register_this_thread(std::size_t capacity_events = 0)` (0 takes the settings' size);
  `static void set_thread_name(std::string_view)`;
  `CalibrationPoint first_point() const noexcept`.
- Thread retirement: the engine's two `thread_local` guards (registration guard and backstop),
  copied with their reasons into the module README.
- No condition variable and no notification: the consumer of P2 polls with an interval it adapts
  from `ThreadDrain::fill` (1 ms when a ring is over half full, doubling to 1 s when idle).

- [ ] Tests: `Small_Collector.OpensOnceAndRefusesASecondOpen`; `Small_Collector.ClosedCollectorRecordsNothing`
      (`recording()` false, `ring()` registers nothing); `Small_Collector.ADrainReturnsWhatThisThreadPushed`;
      `Small_Collector.DropsReachTheDrain` (ring of 64, 70 pushes, `dropped == 6`);
      `Small_Collector.OpenDiscardsEventsFromBefore`; `Medium_Collector.EachThreadHasItsOwnRing` (4 threads, 1,000
      events each, 4 `ThreadDrain`s with 1,000 each); `Medium_Collector.AnExitedThreadIsDrainedThenFreed`
      (thread pushes 10, joins; first drain has its 10 with `retired` true, second drain lacks it);
      `Small_Collector.NamedThreadKeepsItsName`; `Small_Collector.CalibrationPointsAdvance`.
      Each test opens in its fixture and closes plus drains in teardown, so no test sees another's events.
- [ ] Build, test, README, commit `feat(collector): gate recording and hand every ring to one consumer`.

### Task 6: `zones` (capture)

**Files:** `modules/capture/zones/` with `include/SushiProfiler/zones/{zone.hpp,records.hpp,macros.hpp}`,
header only; tests `tests/capture/zones/{test_zone.cpp,test_records.cpp,test_macros.cpp}`; tier
`zones capture`; depends on `collector`.

**Interfaces (produces):**
- `template <bool Compiled> class ScopedZone;` The `false` specialisation is empty and its
  constructor takes and ignores the handle, so a compiled-out zone emits no code.
  `ScopedZone<true>(SiteHandle&)` and `ScopedZone<true>(SiteHandle&, NameID dynamic_name)`: if
  `Collector::recording()` and the site is enabled, read `Collector::now()`; the destructor pushes one `Zone`
  event if it started and the collector still records.
- Free functions, each a no-op while the gate is closed or the site disabled:
  `record_counter(SiteHandle&, double)`, `record_value(SiteHandle&, double)`, `record_instant(SiteHandle&)`,
  `record_instant_at(SiteHandle&, std::uint64_t steady_ns)`, `record_zone_at(SiteHandle&, std::uint64_t begin_ns, std::uint64_t end_ns)`,
  `record_flow(SiteHandle&, EventKind flow_kind, std::uint64_t id)`.
- `template <bool Compiled> class ScopedLane;` writes `LaneBegin` and `LaneEnd` with the lane's
  `NameID`.
- Macros, each taking the compiled level as its first argument so a library writes its own wrapper
  (`#define SR_ZONE(name) SP_ZONE_AT(SR_PROFILE_LEVEL, 1, name, nullptr)`):
  `SP_ZONE_AT(compiled, level, name, group)`, `SP_ZONE_NAMED_AT(compiled, level, name, group, name_id)`,
  `SP_COUNTER_AT(compiled, level, name, unit, op, value)`, `SP_VALUE_AT(compiled, level, name, unit, value)`,
  `SP_INSTANT_AT(compiled, level, name)`, `SP_FLOW_AT(compiled, level, name, kind, id)`,
  `SP_LANE_AT(compiled, lane_id)`; and the direct forms for programs that use SushiProfiler alone,
  `SP_ZONE(name)`, `SP_ZONE_GROUP(name, group)`, `SP_ZONE_FINE(name)`, `SP_COUNTER(name, unit, op, value)`,
  `SP_VALUE(name, unit, value)`, `SP_INSTANT(name)`, `SP_FLOW_BEGIN/STEP/END(name, id)`, `SP_LANE(id)`,
  over `SP_PROFILE_LEVEL` (default 2 when the program does not define it).
- A site is `static constexpr Site` plus `static SiteHandle`, named with `__COUNTER__`.

- [ ] Tests: `Small_Zone.WritesOneCompleteEvent` (kind Zone, begin < end, site's name and line
      read back through `sites().info`); `Small_Zone.NestedZonesNestByTime`; `Small_Zone.ADisabledSiteWritesNothing`;
      `Small_Zone.AZoneClosedAfterTheCollectorClosesWritesNothing`; `Small_Zone.ACompiledOutZoneIsEmpty`
      (`std::is_empty_v<ScopedZone<false>>`, and level 3 under compiled level 1 writes nothing);
      `Small_Records.ACounterKeepsItsOperationAndValue`; `Small_Records.AnExternalZoneKeepsTheCallersTimes`
      (begin 1, end 5'000'000'001 steady ns); `Small_Records.FlowsCarryTheirId`;
      `Small_Records.ALaneBracketsItsEvents`; `Small_Macros.EachDirectMacroWritesItsKind`.
- [ ] Build, test, README, commit `feat(zones): record zones, counters, values, instants, flows and lanes`.

### Task 7: `lock_wait` (capture)

**Files:** `modules/capture/lock_wait/` with `include/SushiProfiler/lock_wait/contended_lock.hpp`,
header only; tests `tests/capture/lock_wait/test_contended_lock.cpp`; tier `lock_wait capture`;
depends on `zones`.

**Interfaces (produces):** `template <class Mutex> class ContendedLock` (a `std::unique_lock`-shaped
guard: `try_lock` first; on failure opens a zone at a `SiteRole::LockWait` site in group
`"Lock wait"`, then `lock`); `SP_LOCK_WAIT_AT(compiled, name, mutex)` and `SP_LOCK_WAIT(name, mutex)`.

- [ ] Tests: `Small_ContendedLock.AnUncontendedLockWritesNothing`;
      `Medium_ContendedLock.AContendedLockWritesOneWaitZone` (a second thread holds the mutex for 20 ms).
- [ ] Build, test, README, commit `feat(lock_wait): record the time a contended lock waits`.

### Task 8: `periods` (capture)

**Files:** `modules/capture/periods/` with `include/SushiProfiler/periods/period_timeline.hpp`,
`source/period_timeline.cpp`; tests `tests/capture/periods/test_period_timeline.cpp`; tier
`periods capture`; depends on `collector`.

**Interfaces (produces):** `class PeriodTimeline`: `explicit PeriodTimeline(std::string_view name, std::uint64_t budget_ns = 0)`;
`std::uint64_t mark() noexcept` and `std::uint64_t mark(std::uint64_t budget_ns) noexcept` (push a
`PeriodMark`, return the index of the period that opens: 0, 1, 2 ...; the index advances even
while the collector is closed, so sources and periods agree); `std::uint64_t current() const noexcept`
(relaxed load); `void tag(std::string_view key, std::string_view value)` (interns both, pushes a
`PeriodTag`; not for a real-time thread); `NameID id() const noexcept`; `std::string_view name() const`.

- [ ] Tests: `Small_PeriodTimeline.MarksCountFromZero`; `Small_PeriodTimeline.AMarkCarriesItsBudget`;
      `Small_PeriodTimeline.TheIndexAdvancesWhileTheCollectorIsClosed`; `Small_PeriodTimeline.ATagCarriesKeyAndValue`;
      `Small_PeriodTimeline.TwoTimelinesKeepTheirOwnIndex`.
- [ ] Build, test, README, commit `feat(periods): mark periods on named timelines`.

### Task 9: Benchmarks, the allocation test and `sp bench`

**Files:** modify `cmake/SushiProfilerOptions.cmake` (`SP_BUILD_BENCHMARKS`, default OFF) and
`cmake/SushiProfilerTests.cmake` (`BENCHMARKS <file>...` in `sushiprofiler_add_module_tests`: each
file becomes a program `sp_bench_<name>` with the target label `benchmark`, never registered with
ctest); `cli/sushiprofiler/` (`build --benchmarks/--no-benchmarks`, `bench` running every
`sp_bench_*` in name order, as SushiBLAS's CLI) with its tests; create
`tests/capture/zones/benchmark_zones.cpp` and `tests/capture/zones/test_zone_allocations.cpp`.

- [ ] Benchmark prints one line per case, `<name> <value> <unit>`, median of 31 runs of 1,000,000
      iterations after 3 warm-up runs: `zone_compiled_out ns`, `zone_gate_closed ns`,
      `zone_site_disabled ns`, `zone_recording ns`, `counter_recording ns`, `clock_now ns`,
      `steady_now ns`. Recording cases drain between runs.
- [ ] `Medium_ZoneAllocations.TheRecordingThreadNeverAllocates`: the test program replaces global
      `operator new` to count per thread; after `register_this_thread` and first use of each site,
      100,000 zones, counters, values, instants, flows and lane scopes allocate 0 times.
- [ ] `sp build --benchmarks --type release`, `sp bench`; record the numbers in this folder's
      REPORT.md against the budget (gate closed under 2 ns; recording under 25 ns). A miss is
      reported, not hidden; the owner decides.
- [ ] CLI tests pass; commit `build(bench): build and run the benchmark programs` and
      `test(zones): measure the hot path and prove it never allocates`.

### Task 10: Close P1

- [ ] `docs/architecture/ARCHITECTURE.md` describes the two tiers and their modules as built;
      `docs/reference/GLOSSARY.md` gains Collector, Gate, Lane, Flow; `docs/reference/CHANGELOG.md` one
      line per commit; module READMEs complete; `ENGINE_CHANGES.md` gains the MSVC TSC row and the
      polling consumer row; design status P1 `built`; `REMAINING_WORK.md` drops RW-1.
- [ ] `write_license_block.py --report` 0 pending; `check_all` clean; full `sp test` green.
- [ ] REPORT.md: commits, test counts, benchmark table, every SOLID breach met or "none".
- [ ] Commit `docs(profiler): close the capture layer`.
