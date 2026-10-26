# SushiProfiler v0.3.0: live session plan

**Status:** Open — written 2026-10-26; nothing built, waiting for the independent review of this plan against the code.
**Implements:** PROFILER#P2c

> **For agentic workers:** REQUIRED SUB-SKILL: use superpowers:subagent-driven-development or superpowers:executing-plans. One task, one build, one acceptance criterion. No worker runs a build; the orchestrator builds at the end of each task it names.

**Goal:** a session that a live tool reads every frame: the newest closed period, a bounded history, retention, writers switched inside a session, pause, every-period windows, per-period facts and per-source delays.

**Architecture:** At each close the consumer thread makes one call to a `PeriodPublisher` of the new `live` module, which builds an immutable `ClosedPeriod` and publishes it to a `LiveChannel` (one latest pointer and a mutex-guarded history ring). `LiveView` is the reader handle `Session::live()` returns; the channel lives as long as the `Session`. Everything else is local: the collector's gate gets a second reason (hold) beside the window, the assembler gets retention, a pause log and a writer set, `Source` gets a delay, and `breakdown`'s arithmetic becomes a free function.

**Spec:** `SPEC.md` in this folder, amended in the same commit as this plan where the code contradicted it (rows for names at run time, the pause and the live read; see D2, D3, D5, D9). Its other decisions are not reopened. Every choice it does not cover is decided under "Decisions I took", with a reason. The independent review of 2026-10-26 ruled V1 to V13; each ruling is a decision below (D19 to D30) or a change in its task.

## Global constraints

- C++17, no C++20 library or language feature. `/W4 /WX` clean on MSVC, `-Wall -Wextra -Werror` on GCC and Clang. Allman, 4 spaces, 100 columns, `cpp-code-style` naming.
- Hot-path budgets from `docs/design/PROFILER/README.md`: zone with the gate closed under 2 ns (one relaxed load and one branch); recording zone under 25 ns; no allocation after `register_this_thread`; idle drainer at most one wake-up a second. No task adds a load, a branch or an include to `Zone`, `ScopedZone` or `ContendedLock`.
- Rebuild cost: `zone.hpp`, `contended_lock.hpp`, `site.hpp` and `trace_event.hpp` gain no include and no member. New macros live in new headers. `collector.hpp` gains declarations only, no include.
- No dependency but the C++17 library and Threads. SushiProfiler logs nothing and returns statuses.
- Every new module, header and source file carries the license block and the `@file` block (`source-comments`). Test sizes `Small_`, `Medium_` in the suite name, fixed seeds, no wall-clock assertions: a test that waits uses a condition with a 5 s timeout and asserts on the condition.
- No task edits a build file. Each task lists the build lines the orchestrator adds before it builds.
- Prose in the module READMEs and manual pages follows `humanizer`: no em dash, no `previously`, `no longer`, `new in`.

## Decisions I took

Each one the SPEC leaves open; the reason is one line.

| # | Decision | Reason |
| --- | --- | --- |
| D1 | The linger key stays `buffers.linger_ms`. The SPEC's `consumer.linger_ms` is read as the same setting. `docs/guides/CLIENT_LIBRARY.md` line 97 says `consumer.linger_ms` and is corrected | Renaming a published key breaks every config file; `profiler_config.hpp` and `CONFIG_FORMAT.md` hold `buffers.linger_ms` |
| D2 | (SPEC amended) Run-time-named counters and values are new macros `SP_COUNTER_NAMED_AT`, `SP_VALUE_NAMED_AT` and their level-defaulted forms, not a change to `SP_COUNTER_AT`/`SP_VALUE_AT`. RW-13's "done when" is met by them | `SP_COUNTER_AT` puts its name in a `static constexpr Site`; a run-time string cannot go there, and a macro cannot overload on type in C++17 |
| D3 | (SPEC amended) `SP_ZONE_NAMED_AT` does not intern: it takes an interned `name_id` (`macros.hpp:67`). The named counters therefore do not copy it. Each distinct name becomes its own site in a `DynamicSiteTable`, so the record format and `SampleSeries` do not change | A name field on `SampleSeries` is a record format change, which is the owner's decision |
| D4 | A `DynamicSiteTable` holds at most 4096 names; later names are refused and counted | A name built from a frame number would fill the 65 536-site registry the static sites share |
| D5 | (SPEC amended) The collector gets two reasons to close the gate. The window's `pause`/`resume` set `marking_`; `hold`/`release` set `held_`; the zone gate is `marking_ && !held_`. A period mark is written whenever `marking_` is set, so periods keep their boundaries while held | One shared flag would let the window's next `resume` undo a user pause; without marks a pause would merge periods into one |
| D6 | A pause is recorded by the session in a `PauseLog` of steady-clock intervals. A period that overlaps one gets the fact `paused` = `true` in `PeriodRecord::facts`. `PeriodRecord` gains no field | The SPEC says it gains nothing; facts exist and are written by the record writer |
| D7 | Live defaults: `live.history` 300, `live.max_runs` 2048, `live.min_run_ns` 1000, `live.max_poll_ms` 4. Ranges in L5. When runs are capped, the shortest are dropped first and the rest keep their (thread, lane, begin) order; the count omitted is kept | 300 matches the editor's history; 2048 runs of 40 bytes bound a period near 80 KB |
| D8 | `ClosedPeriod` carries cumulative late, dropped, span-dropped and outside counts as of its close, not per-period counts | A late event is counted after its period closed, so a per-period count would be wrong when published |
| D9 | (SPEC amended) `LiveView::latest()` takes a short library lock: it is `std::atomic_load` on a `std::shared_ptr`, a refcount copy under the standard library's internal lock, not one lock-free load. The docs say so | C++17 has no lock-free atomic shared pointer; a hazard scheme adds a module for no reader gain |
| D10 | The consumer publishes only while a `LiveView` exists (`LiveChannel::watched()`). Creating one wakes the consumer; while one exists the consumer's slowest poll is `live.max_poll_ms` | A session nobody watches pays no copy per period and keeps the one wake-up a second |
| D11 | `window.active` is `"all"` or a whole number. With `"all"`, `repeat` must be absent or 1 (refused otherwise), `skip`, `wait` and `warmup` apply once, and the window never reaches `Done`. `WindowSchedule` gains `bool active_all` | A repeat of a cycle that never ends has no meaning |
| D12 | A writer's `enabled` key is read by the config reader into `NamedSettings::enabled` and removed from the settings object the factory sees. Only stream writers switch live. A writer that starts disabled is `begin`-ed on the consumer thread when first enabled. `end` runs once for every writer that began | `output_path` refuses unknown keys; record writers run once at stop, so a switch means nothing for them |
| D13 | A source's `max_delay_ns()` above 60 000 ms is refused at `Session::start`, naming the source | Same bound as `buffers.linger_ms`; the assembler holds every event of the wait in memory |
| D14 | Retention trims in batches: with `keep_periods` N the record holds up to 2N periods while running and exactly N at stop; N = 0 keeps none, never pushing. The assembler's closed-span list is coalesced and capped at 4096 entries, its reopen list at 4096. An event older than the tracked spans counts as outside | Erasing the front of a vector per period is O(N); both lists grow without bound on a long window today |
| D15 | The process-wide dynamic tables are exported free functions in `sites`, not `Collector` members | `collector.hpp` stays unchanged for consumers |
| D16 | `SP_LOCK_WAIT_NAMED` and its helper live in a new header `lock_wait/named_lock_wait.hpp`; named counter macros in `zones/named_macros.hpp`. `SP_THREAD_NAME` goes in `macros.hpp` as the design says: it needs no include `macros.hpp` lacks | Keeps `contended_lock.hpp` and the other widely included headers unchanged |
| D17 | `Session::pause()` and `resume()` return `bool`: whether they took effect (false when the session is disabled or already in that state). `set_writer_enabled` returns `WriterSwitch` | Statuses, not exceptions |
| D18 | `SOVERSION` and the project version move to 0.3 at release | Pre-1.0 minor for a larger public surface (`versioning-and-release`) |
| D19 | (V1) The pause log answers `ends_after(steady_ns)`; the assembler appends each pause end, in record time, to `reopened_ns_`, so `queue` resets a thread's lanes across a pause the way it does across a window gap. The session logs `begin` before `hold` and `end` before `release` | A lane open across a hold has lost its end event; without the reset the lane runs on |
| D20 | (V2) `Session` holds one mutex that serialises `pause`, `resume`, `set_writer_enabled`, `live`, `report`, `stop` and `restart`, documented in `session.hpp`. `timeline()` keeps its own mutex | Control calls come from a UI thread while another thread stops or restarts |
| D21 | (V3) `Session` owns one `LiveChannel` for its whole life. A new member `std::string restart()` starts the next run on the same object after `stop()`: it resets every timeline to period 0 (`PeriodTimeline::reset()`), calls `LiveChannel::begin_run()`, which clears the history and the latest pointer and bumps the run number `ClosedPeriod::run`, and runs `open()` again. The static `start` is unchanged | The SPEC wants a view taken before `stop` to keep working after the next start; the static factory makes a new object, so a same-object restart is needed. Timelines must restart or the window's index schedule would continue from the old run |
| D22 | (V4) `close` order: fold, collect, stream writers, append to the record, `publisher.closed(...)` from the appended period, then trim | `keep_periods` 0 must still publish; nothing is published before the writers and the record have the period |
| D23 | (V5) `PeriodPublisher` in `live` owns the `NameSnapshotCache`, the sequence and the `RunCap`; `close` makes one call to it. `PeriodAssembler` takes its collaborators in one `AssemblyParts` struct of references (`const Window&`, `const PauseLog&`, `PeriodPublisher&`, `StreamWriterSet&`) that must outlive it; `AssemblySettings` holds values only. A session without a timeline passes a `Window` with `active_all`, which the assembler never consults | Dependencies arrive through the constructor, and a bag of optional raw pointers hides which collaborator is missing |
| D24 | (V8) Every close drops `reopened_ns_` entries at or below the smallest `last_write_ns` of the threads that have not retired, and asks the pause log to forget intervals that ended before the smaller of that time and the oldest open period's begin. Both lists keep the 4096 cap | A long run with idle threads still stays bounded; a retired thread never writes again so it must not hold the minimum back |
| D25 | (V9) `SpanSink` keeps the three-argument constructor; a sink built with it has no facts vector and `set_fact` returns false and counts a refusal. `set_fact` returns `bool` | A v0.2.0 program builds a sink itself and must still compile |
| D26 | (V10) One ungrouped label, the public `K_UNGROUPED_GROUP`; `record_queries` uses it and its own `K_UNGROUPED` goes | Two copies of one string drift |
| D27 | (V12) `state_facts` writes `config.record.keep_periods` (`all` when absent) | A record that kept the last N periods must say so |
| D28 | `docs`: the `lock_wait` README states that `SP_LOCK_WAIT_NAMED` measures the first acquisition only, not re-locks inside `std::condition_variable::wait` | A reader would otherwise read the zone as total wait |
| D29 | (V7) The level-0 behaviour of `SP_THREAD_NAME` and the named macros is tested in its own unit, `tests/capture/zones/test_named_macros.cpp`, with `SP_PROFILE_LEVEL` 0. It is named for the header it mirrors, `named_macros.hpp`, because `check_test_mirror.py` refuses a test that mirrors no file | A macro cannot be both levels in one translation unit |
| D31 | (V14) `Collector::sources_wanted()` is true exactly while a session runs and is not paused: `start` sets it true after its sources began, `pause` sets it false, `resume` sets it true, `stop` sets it false before its sources end. It is documented in the collector README and the live-view guide | A source that pays for device timing should stop paying while the session records nothing |
| D30 | (V13) L1 is accepted on its own tests; the zone budget comparison belongs to L14 | L1 changes no `ScopedZone` code |

## Review focus

The five failure modes most likely to bite, most likely first. Each has a named test in the task that owns it.

1. **A reader holds an old `ClosedPeriod` while the ring wraps or the session ends.** Expected: the period it holds is intact and its names still resolve. Tests: `Small_LiveChannel.AReaderKeepsAnOverwrittenPeriodAlive` (L8), `Medium_Session.ALiveViewOutlivesTheSessionAndKeepsItsLastPeriods` (L13).
2. **Publication races period close and retention.** Expected: `history(n)` is consecutive by `sequence`, `latest()` is never behind the history it was read after, and nothing is published before the period is final (writers done, record appended). Tests: `Medium_LiveChannel.ReadersNeverSeeSequenceGoBackwards`, `Small_PeriodAssembler.PublishesAfterTheWritersAndTheRecord` (L8, L12).
3. **A writer is toggled during a close.** Expected: a request posted while a period closes takes effect at the next close; a writer never gets `write_period` before `begin` or after `end`. Tests: `Medium_StreamWriterSet.RequestsFromAnotherThreadNeverTearAPeriod`, `Small_PeriodAssembler.ATogglePostedWhileAPeriodIsOpenTakesEffectAtTheNextClose` (L11).
4. **Pause across a period boundary.** Expected: every period that overlaps the pause carries `paused`, none that only touches it does, a resumed window records again, and period indices stay continuous. Tests: `Small_PauseLog.ABoundaryTouchIsNotAnOverlap`, `Medium_Session.PauseMarksTheCrossedPeriodsAndResumeRecordsAgain` (L10, L13).
5. **A source delay larger than the session can hold.** Expected: a delay above 60 s is refused at start; a delay of several periods keeps that many periods open, closes them in order and loses no span or event. Tests: `Small_Session.RefusesASourceDelayAboveTheLimit`, `Medium_Session.ASourceDelayHoldsPeriodsOpenAndLosesNone` (L13).

Also pinned: a name explosion fills a `DynamicSiteTable` and stops at 4096 (`L2`); a long windowed run keeps the assembler's lists bounded (`L9`); `"all"` with `repeat` 2 is refused (`L5`).

## File map

| Tier | Module | Change |
| --- | --- | --- |
| capture | `collector` | `hold`, `release`, `marking`, `mark_ring`, `sources_wanted` (set by the session, D31) |
| capture | `periods` | marks written through `mark_ring` while `marking` |
| capture | `sites` | `DynamicSiteTable` and the two process tables |
| capture | `zones` | `named_macros.hpp`, `named_records.hpp`, `SP_THREAD_NAME` |
| capture | `lock_wait` | `named_lock_wait.hpp` |
| record | `analysis` | `period_breakdown.hpp`; `breakdown` calls it |
| session | `config` | `active: "all"`, `record`, `live`, writer `enabled` |
| session | `window` | every-period schedule |
| session | `sources` | `Source::max_delay_ns`, `SpanSink::set_fact` |
| session | `live` (new) | `NameSnapshot`, `ClosedPeriod`, `LiveChannel`, `LiveView` |
| session | `consumer` | retention, `PauseLog`, `StreamWriterSet`, publication, poll ceiling |
| session | `session` | `live`, `pause`, `resume`, `set_writer_enabled`, delays, plumbing |

Tasks run in this order. L1 to L4 and L5 to L7 do not depend on each other and may run in parallel; L8 needs L4 only through its README; L9 to L12 are sequential; L13 needs all before it.

---

### L1: The collector's second gate reason and the sources flag

**Files:**
- Modify: `modules/capture/collector/include/SushiProfiler/collector/collector.hpp` (declarations only, no include), `modules/capture/collector/source/collector.cpp`
- Modify: `modules/capture/periods/source/period_timeline.cpp` (`write_mark`)
- Modify: `modules/capture/collector/README.md`, `modules/capture/periods/README.md`
- Test: `tests/capture/collector/test_collector.cpp`, `tests/capture/periods/test_period_timeline.cpp`

**Interfaces produced** (all in `Collector`):
```cpp
void hold();                                        // closes the zone gate until release(); no-op while closed
void release();                                     // reopens it where the window wants it
[[nodiscard]] bool is_held() const noexcept;
[[nodiscard]] static bool marking() noexcept;       // one relaxed load; true while the window records
[[nodiscard]] static ThreadRing* mark_ring() noexcept; // like ring(), registers while marking()
[[nodiscard]] static bool sources_wanted() noexcept;   // one relaxed load
static void want_sources(bool wanted) noexcept;
```
Statics `marking_` and `sources_wanted_` are `std::atomic` beside `gate_`. The gate is `marking_ && !held_`, recomputed under `mutex_` by `open` (marking 1, held 0), `close` (all 0, held cleared), `pause` (marking 0), `resume` (marking 1 when open), `hold`, `release`. `recording()` and `ring()` are unchanged. `write_mark` tests `marking()` and takes its ring from `mark_ring()`; `tag` keeps `recording()`, so tags during a hold are dropped.

**Tests:**
- `Small_Collector.HoldClosesTheZoneGateButKeepsMarking`: after open and hold, `recording()` is false and `marking()` is true.
- `Small_Collector.ReleaseReopensTheGateOnlyWhereTheWindowWants`: pause, hold, release leaves `recording()` false; resume then reopens it; hold then resume leaves it false until release.
- `Small_Collector.CloseClearsAHold`: hold, close, open gives `recording()` true and `is_held()` false.
- `Small_Collector.SourcesWantedFollowsWantSources`: false by default, true after `want_sources(true)`, false after `want_sources(false)`.
- `Small_PeriodTimeline.AMarkWrittenWhileHeldReachesTheDrain`: a mark during a hold is in the drain, a zone opened in the same period is not.
- `Small_PeriodTimeline.ATagWhileHeldIsDropped`: no `PeriodTag` in the drain.
- `Medium_PeriodTimeline.AMarkFromAnUnregisteredThreadWhileHeldRegistersItsRing`: a new thread marks during a hold; the drain has its ring and its mark.

**Build lines:** none (no new file).

**Docs and changelog:** the two READMEs describe the two reasons and `mark_ring`. Add under Unreleased: `- 2026-10-26 — collector: Added hold and release, a second reason to close the zone gate that leaves period marks written, and sources_wanted (modules/capture/collector/, modules/capture/periods/).`

**Acceptance:** the seven tests pass (D30: the zone budget comparison is L14's).

---

### L2: Names built at run time, and `SP_THREAD_NAME` (RW-13)

**Files:**
- Create: `modules/capture/sites/include/SushiProfiler/sites/dynamic_site_table.hpp`, `modules/capture/sites/source/dynamic_site_table.cpp`
- Create: `modules/capture/zones/include/SushiProfiler/zones/named_records.hpp`, `.../named_macros.hpp`
- Modify: `modules/capture/zones/include/SushiProfiler/zones/macros.hpp` (add `SP_THREAD_NAME` only)
- Modify: `modules/capture/sites/README.md`, `modules/capture/zones/README.md`, `tests/capture/zones/benchmark_zones.cpp`
- Test: `tests/capture/sites/test_dynamic_site_table.cpp`, `tests/capture/zones/test_named_records.cpp`, `tests/capture/zones/test_macros.cpp`

**Interfaces produced:**
```cpp
class SUSHIPROFILER_API DynamicSiteTable final
{
    public:
        static constexpr std::size_t K_MAX_NAMES = 4096U;
        explicit DynamicSiteTable(SiteRole role);
        [[nodiscard]] SiteHandle* handle(std::string_view name, std::string_view unit,
                                         CounterOp op, std::uint8_t level);
        [[nodiscard]] std::size_t size() const;
        [[nodiscard]] std::uint64_t refused() const noexcept;
};
SUSHIPROFILER_API DynamicSiteTable& dynamic_counter_sites();   // SiteRole::Counter
SUSHIPROFILER_API DynamicSiteTable& dynamic_value_sites();     // SiteRole::Value
```
`handle` returns a stable pointer, one per name; null past `K_MAX_NAMES`. The table owns each name and unit in a node of `std::map<std::string, Entry, std::less<>>` under a `std::shared_mutex` (shared lock to find, unique lock to add), so a lookup by `string_view` allocates nothing. The first unit and operation of a name win. `named_records.hpp`: `record_counter_named(std::string_view name, std::string_view unit, CounterOp op, std::uint8_t level, double value) noexcept` and `record_value_named(std::string_view name, std::string_view unit, std::uint8_t level, double value) noexcept`; both catch `std::bad_alloc` and drop. `named_macros.hpp`:
```cpp
SP_COUNTER_NAMED_AT(compiled, level, name_expression, unit, op, value)
SP_VALUE_NAMED_AT(compiled, level, name_expression, unit, value)
SP_COUNTER_NAMED(name_expression, unit, op, value)   // SP_PROFILE_LEVEL, level 1
SP_VALUE_NAMED(name_expression, unit, value)
```
The expansion tests `Collector::recording()` first and evaluates `name_expression` only inside that branch. `macros.hpp` gains `SP_THREAD_NAME(name)`: `::SushiProfiler::Collector::set_thread_name(name)` at `SP_PROFILE_LEVEL >= 1`, `static_cast<void>(0)` otherwise.

**Tests:**
- `Small_DynamicSiteTable.ReturnsOneStableHandlePerName`: two calls, one pointer; two names, two pointers.
- `Small_DynamicSiteTable.KeepsTheFirstUnitAndOperationOfAName`: the second call's unit and op are ignored.
- `Small_DynamicSiteTable.CopiesTheNameSoATemporaryMayBeFreed`: the registry's `info` still reads the name after the caller's string is destroyed.
- `Small_DynamicSiteTable.RefusesNamesPastTheLimitAndCountsThem`: name 4097 returns null and `refused()` is 1.
- `Medium_DynamicSiteTable.ManyThreadsGetTheSameHandle`: eight threads, 1000 calls each over 10 names, 10 handles.
- `Small_NamedRecords.ACounterNamedAtRunTimeFoldsPerName`: two names in a recording drain are two sites with those names and the counter operation.
- `Small_NamedRecords.AValueNamedAtRunTimeKeepsEverySample`.
- `Small_NamedRecords.ANameIsNotBuiltWhileTheGateIsClosed`: the name expression increments a counter; it stays 0 with the gate closed.
- `Small_NamedRecords.NamesAreKeptPerRole`: a counter and a value of one name are two sites.
- `Small_Macros.ThreadNameNamesTheRing`: the drain's thread name is the one set.
- `Small_NamedMacros.ThreadNameIsCompiledOutAtLevelZero`: in the new unit `tests/capture/zones/test_named_macros.cpp`, which defines `SP_PROFILE_LEVEL` 0, the macro leaves the ring unnamed and `SP_COUNTER_NAMED` does not evaluate its name (D29).
- Benchmark line `counter_named_recording <ns> ns` added to `benchmark_zones.cpp`; it is outside the 25 ns row and says so in its output.

**Build lines:** `modules/capture/sites/CMakeLists.txt` SOURCES `dynamic_site_table.cpp`; `tests/capture/sites/CMakeLists.txt` SOURCES `test_dynamic_site_table.cpp`; `tests/capture/zones/CMakeLists.txt` SOURCES `test_named_records.cpp` and `test_named_macros.cpp`. Dependencies unchanged.

**Docs and changelog:** zones README (macros table, cost of a named record: one shared lock and one map lookup, not for a real-time thread), sites README, `docs/design/PROFILER/CAPTURE_AND_SESSION.md` macro line is updated at release (L15). Changelog: `- 2026-10-26 — zones: Added SP_COUNTER_NAMED and SP_VALUE_NAMED, which take a run-time name, and SP_THREAD_NAME (modules/capture/zones/, modules/capture/sites/).`

**Closes:** RW-13.

**Acceptance:** the eleven tests pass; `Small_Zone` allocation tests are unchanged and green.

---

### L3: A lock that passes to a condition variable

**Files:**
- Create: `modules/capture/lock_wait/include/SushiProfiler/lock_wait/named_lock_wait.hpp`
- Modify: `modules/capture/lock_wait/README.md`
- Test: `tests/capture/lock_wait/test_named_lock_wait.cpp`

**Interfaces produced:**
```cpp
template <class Mutex, bool Compiled>
void lock_with_wait(std::unique_lock<Mutex>& lock, SiteSlot<Compiled>& handle);
// tries lock.try_lock(); on failure opens a ScopedZone<Compiled> and calls lock.lock()

SP_LOCK_WAIT_NAMED_AT(compiled, lock, name, mutex)
SP_LOCK_WAIT_NAMED(lock, name, mutex)   // SP_PROFILE_LEVEL
```
The macro declares the site (`SP_DETAIL_SITE`, group "Lock wait", level 1, `LockWait`) and `std::unique_lock<std::remove_reference_t<decltype(mutex)>> lock(mutex, std::defer_lock)`, then calls `lock_with_wait`. `lock` is a non-const local named by the caller, so `std::condition_variable::wait(lock, ...)` takes it. `contended_lock.hpp` is not edited.

**Tests:**
- `Small_NamedLockWait.AnUncontendedLockWritesNothing`: the drain holds no event.
- `Medium_NamedLockWait.AContendedLockRecordsOneLockWaitZone`: a second thread holds the mutex; the waiter's drain holds one `LockWait` zone.
- `Medium_NamedLockWait.TheLockPassesToAConditionVariableWait`: the declared lock waits on a `std::condition_variable`; another thread sets the flag and notifies; the wait returns with the lock held.
- `Small_NamedLockWait.ACompiledOutLockStillLocks`: at compiled level 0 the lock is held and nothing is recorded.

**Build lines:** `tests/capture/lock_wait/CMakeLists.txt` SOURCES `test_named_lock_wait.cpp`.

**Docs and changelog:** lock_wait README, which states that the zone measures the first acquisition only, not re-locks inside `std::condition_variable::wait` (D28). `- 2026-10-26 — lock_wait: Added SP_LOCK_WAIT_NAMED, which declares a std::unique_lock a condition variable can wait on (modules/capture/lock_wait/).`

**Acceptance:** the four tests pass.

---

### L4: Per-period breakdown as a free function

**Files:**
- Create: `modules/record/analysis/include/SushiProfiler/analysis/period_breakdown.hpp`, `modules/record/analysis/source/period_breakdown.cpp`
- Modify: `modules/record/analysis/source/record_queries.cpp` (`group_series` calls it; `group_of` shares one helper over a site table), `modules/record/analysis/README.md`
- Test: `tests/record/analysis/test_period_breakdown.cpp`

**Interfaces produced:**
```cpp
inline constexpr const char* K_UNGROUPED_GROUP = "(ungrouped)";
struct GroupSelfTime { std::string group; std::uint64_t self_ns = 0U; };
struct ThreadTotal { std::uint32_t thread = 0U; std::uint64_t root_inclusive_ns = 0U;
                     std::uint64_t self_ns = 0U; };
struct PeriodBreakdown
{
    std::vector<GroupSelfTime> groups;  // marking thread's trees; self_ns descending, then name
    std::uint64_t uncovered_ns = 0U;    // max(0, duration less the marking thread's root time)
    std::vector<ThreadTotal> threads;   // every thread, lanes summed, ascending index
};
[[nodiscard]] SUSHIPROFILER_API PeriodBreakdown break_down_period(
    const PeriodRecord& period, const std::vector<SiteRecord>& sites);
```
A node's group is its site's, else its nearest grouped ancestor's, else `K_UNGROUPED_GROUP`; `record_queries.cpp` uses this one constant and its own `K_UNGROUPED` is removed (D26). The `breakdown` analysis output is unchanged byte for byte.

**Tests** (`Small_PeriodBreakdown`):
- `InheritsTheNearestGroupedAncestorsGroup`.
- `ChargesUngroupedZonesToTheUngroupedLabel`.
- `CountsOnlyTheMarkingThreadsRootsAsCovered`.
- `UncoveredIsZeroWhenZonesOverrunThePeriod`.
- `ReportsEveryThreadsTotalsWithLanesSummed`.
- `MatchesTheBreakdownAnalysisOnOnePeriod`: its group self times equal the analysis table's medians for a one-period record.
- Existing `Small_BreakdownAnalysis` tests unchanged and green.

**Build lines:** `modules/record/analysis/CMakeLists.txt` SOURCES `period_breakdown.cpp`; `tests/record/analysis/CMakeLists.txt` SOURCES `test_period_breakdown.cpp`.

**Docs and changelog:** analysis README. `- 2026-10-26 — analysis: Added break_down_period, which gives one period's self time per group, uncovered time and per-thread totals; breakdown calls it (modules/record/analysis/).`

**Acceptance:** the six tests pass and every existing analysis test is unchanged.

---

### L5: Configuration keys

**Files:**
- Modify: `modules/session/config/include/SushiProfiler/config/profiler_config.hpp`, `modules/session/config/source/profiler_config.cpp`, `modules/session/config/README.md`
- Modify: `docs/reference/CONFIG_FORMAT.md`
- Test: `tests/session/config/test_profiler_config.cpp`

**Interfaces produced** (`<optional>` already included):
```cpp
struct WindowSchedule { /* existing */ bool active_all = false; };
struct RecordSettings { std::optional<std::uint32_t> keep_periods; };   // absent keeps all
struct LiveSettings { std::uint32_t history = 300U;       // 1 to 65 536
                      std::uint32_t max_runs = 2048U;     // 0 to 2^24, 0 keeps no run
                      std::uint64_t min_run_ns = 1000U;   // 0 to 1 000 000 000
                      std::uint32_t max_poll_ms = 4U; };  // 1 to 1000
struct NamedSettings { std::string name; JsonValue settings; bool enabled = true; };
struct ProfilerConfig { /* existing */ RecordSettings record; LiveSettings live; };
```
Reader: top-level keys `record` (`keep_periods`, 0 to 2^32 - 1) and `live`; `window.active` is a whole in 1 to 2^32 - 1 or the string `"all"`; with `"all"`, `repeat` other than 1 is refused with its path; for `writers` only, an `enabled` bool member is read into `NamedSettings::enabled` and removed from `settings`. The format version stays 1: every new key is optional.

**Tests** (`Small_ProfilerConfig`):
- `ReadsActiveAll`; `RefusesAnActiveStringOtherThanAll`; `RefusesAllWithARepeatOfTwo` (path `window.repeat`); `AcceptsAllWithRepeatOne`.
- `ReadsKeepPeriodsIncludingZeroAndLeavesItAbsentByDefault`.
- `ReadsTheLiveKeysAndKeepsTheirDefaults`: `live` absent gives 300, 2048, 1000, 4.
- `RefusesALiveKeyOutOfRange`: `live.max_poll_ms` 0 and 1001, `live.history` 0; each refusal names its path.
- `ReadsAWriterEnabledFlagAndHidesItFromTheSettings`: `settings` has no `enabled` member and `enabled` is false.
- `LeavesASourcesEnabledKeyInItsSettings`.
- `RefusesAnUnknownRecordKey`.

**Build lines:** none.

**Docs and changelog:** `CONFIG_FORMAT.md` key table, `window` table (`active` takes `"all"`) and example gain the keys; the README lists them. `- 2026-10-26 — config: Added window.active "all", record.keep_periods, the live keys and a writer's enabled flag (modules/session/config/, docs/reference/CONFIG_FORMAT.md).`

**Acceptance:** the eleven tests pass; every existing config test is unchanged.

---

### L6: A window of every period (RW-19)

**Files:**
- Modify: `modules/session/window/source/window.cpp`, `modules/session/window/include/SushiProfiler/window/window.hpp` (comment only), `modules/session/window/README.md`
- Test: `tests/session/window/test_window.cpp`

**Interfaces:** `Window::phase_of` and `records` unchanged in signature. With `active_all`: periods below `skip` are `Skipped`, the next `wait` are `Waiting`, the next `warmup` are `Warmup`, every later period is `Active`; there is no `Done` and no modulo.

**Tests** (`Small_WindowPhases`):
- `ActiveAllRecordsEveryPeriodAfterWarmup`: for indices up to 10 000 000, none is `Done`.
- `ActiveAllAppliesSkipWaitAndWarmupOnce`.
- `ActiveAllRecordsFromTheFirstPeriodWithNoWarmup`.

**Build lines:** none.

**Docs and changelog:** window README. `- 2026-10-26 — window: Added the every-period schedule, window.active "all" (modules/session/window/).`

**Closes:** RW-19 with L13's end-to-end test.

**Acceptance:** the three tests pass; existing window tests unchanged.

---

### L7: Facts and delays from sources (RW-7)

**Files:**
- Modify: `modules/session/sources/include/SushiProfiler/sources/source.hpp`, `.../span_sink.hpp`, `modules/session/sources/source/span_sink.cpp`
- Modify: `modules/session/consumer/source/period_assembler.cpp` (the one `SpanSink` construction in `close`)
- Modify: `modules/session/sources/README.md`, `docs/guides/CLIENT_LIBRARY.md`
- Test: `tests/session/sources/test_span_sink.cpp`, `tests/session/sources/test_source.cpp`, `tests/session/consumer/test_period_assembler.cpp`

**Interfaces produced:**
```cpp
// Source
[[nodiscard]] virtual std::uint64_t max_delay_ns() const noexcept { return 0U; }

// SpanSink: the facts vector is the period's
SpanSink(SpanTable& table, std::vector<Fact>& facts, const NameTable& names,
         const std::vector<std::string>& tracks) noexcept;
SpanSink(SpanTable& table, const NameTable& names,
         const std::vector<std::string>& tracks) noexcept;   // v0.2.0 form: no facts
[[nodiscard]] bool set_fact(std::string_view name, std::string_view value);
// replaces a fact of that name; false (counted as refused) for an empty name or a sink with no facts (D25)
```
 `close` builds the sink with `period.facts`. A source states its delay once, from `begin` onward; the session reads it after `begin` succeeds.

**Tests:**
- `Small_SpanSink.SetsAFactOnThePeriod`; `ReplacesAFactOfTheSameName`; `RefusesAnEmptyFactName`; `RefusesAFactOnASinkBuiltWithoutFacts`.
- `Small_Source.ASourceStatesNoDelayByDefault`; `AnOverridingSourceStatesItsDelay`.
- `Small_PeriodAssembler.ASourceFactReachesThePeriodRecord`.

**Build lines:** none.

**Docs and changelog:** the sources README and the guide gain the delay and fact paragraphs; the guide's last line is corrected to `buffers.linger_ms` (D1). `- 2026-10-26 — sources: Added Source::max_delay_ns and SpanSink::set_fact; SpanSink's constructor takes the period's facts (modules/session/sources/, docs/guides/CLIENT_LIBRARY.md).` The three-argument constructor stays, so no program breaks (D25).

**Closes:** RW-7 with L13's file round trip.

**Acceptance:** the six tests pass; the assembler tests that build sinks are unchanged.

---

### L8: The `live` module

**Files (all create):**
- `modules/session/live/CMakeLists.txt`, `modules/session/live/README.md`
- `include/SushiProfiler/live/name_snapshot.hpp`, `closed_period.hpp`, `live_channel.hpp`, `live_view.hpp`, `period_publisher.hpp` and their sources under `modules/session/live/source/`
- `tests/session/live/CMakeLists.txt`, `README.md`, `test_name_snapshot.cpp`, `test_closed_period.cpp`, `test_live_channel.cpp`, `test_live_view.cpp`, `test_period_publisher.cpp`
- `docs/guides/LIVE_VIEW.md`

**Interfaces produced:**
```cpp
class SUSHIPROFILER_API NameSnapshot final
{
    public:
        NameSnapshot(std::vector<std::string> names, std::vector<SiteRecord> sites,
                     std::vector<std::string> tracks, std::vector<ThreadRecord> threads);
        [[nodiscard]] std::string_view name(NameID id) const noexcept;
        [[nodiscard]] const SiteRecord* site(SiteID id) const noexcept;
        [[nodiscard]] std::string_view track(TrackID id) const noexcept;
        [[nodiscard]] std::string_view thread_name(std::uint32_t index) const noexcept;
        [[nodiscard]] const std::vector<SiteRecord>& sites() const noexcept;   // for break_down_period
};

class SUSHIPROFILER_API NameSnapshotCache final
{
    public:
        /// Returns the last snapshot while names, sites, tracks and thread names are unchanged.
        [[nodiscard]] std::shared_ptr<const NameSnapshot> snapshot(const RunRecord& record);
};

struct LiveCounts { std::uint64_t late = 0U; std::uint64_t dropped = 0U;
                    std::uint64_t spans_dropped = 0U; std::uint64_t outside = 0U; };
struct RunCap { std::uint32_t max_runs = 0U; std::uint64_t min_run_ns = 0U; };

struct ClosedPeriod
{
    std::uint64_t run = 0U;               // the Session run this period belongs to, from 1
    std::uint64_t sequence = 0U;          // 0 for the first period of its run
    std::uint64_t origin_steady_ns = 0U;  // add to a record time to get the steady clock
    PeriodRecord period;                  // runs capped by RunCap
    std::uint64_t runs_omitted = 0U;
    LiveCounts counts;                    // cumulative as of this close (D8)
    std::shared_ptr<const NameSnapshot> names;
};

[[nodiscard]] SUSHIPROFILER_API std::shared_ptr<const ClosedPeriod> make_closed_period(
    const PeriodRecord& period, std::uint64_t run, std::uint64_t sequence,
    std::uint64_t origin_steady_ns, const RunCap& cap, const LiveCounts& counts,
    std::shared_ptr<const NameSnapshot> names);

class SUSHIPROFILER_API LiveChannel final
{
    public:
        explicit LiveChannel(std::size_t history_capacity);   // at least 1
        std::uint64_t begin_run();     // clears history and latest, run number + 1; returns it
        [[nodiscard]] std::uint64_t run() const noexcept;     // 0 before the first begin_run
        void publish(std::shared_ptr<const ClosedPeriod> period);   // consumer thread only
        [[nodiscard]] std::shared_ptr<const ClosedPeriod> latest() const noexcept;
        [[nodiscard]] std::vector<std::shared_ptr<const ClosedPeriod>> history(
            std::size_t count) const;                          // oldest first, at most count
        [[nodiscard]] bool watched() const noexcept;           // a LiveView exists
        void attach() noexcept;
        void detach() noexcept;
        [[nodiscard]] std::uint64_t published() const noexcept;   // since begin_run
};

class SUSHIPROFILER_API LiveView final     // copyable handle; copies count as viewers
{
    public:
        LiveView() noexcept;
        explicit LiveView(std::shared_ptr<LiveChannel> channel);
        LiveView(const LiveView&); LiveView& operator=(const LiveView&);
        LiveView(LiveView&&) noexcept; LiveView& operator=(LiveView&&) noexcept;
        ~LiveView();
        [[nodiscard]] bool valid() const noexcept;
        [[nodiscard]] std::shared_ptr<const ClosedPeriod> latest() const noexcept;   // null before the first
        [[nodiscard]] std::vector<std::shared_ptr<const ClosedPeriod>> history(
            std::size_t count) const;
};

/// The one object the assembler calls at a close (V5): it owns the snapshot cache, the sequence
/// and the cap, and does nothing while the channel is not watched.
class SUSHIPROFILER_API PeriodPublisher final
{
    public:
        PeriodPublisher(LiveChannel& channel, const RunCap& cap, std::uint64_t origin_steady_ns);
        void closed(const PeriodRecord& period, const RunRecord& record);
};
```
`LiveChannel::publish` takes the history mutex, stores the pointer in the ring, `std::atomic_store`s `latest_`, and unlocks. `PeriodPublisher::closed` returns at once when `channel.watched()` is false; otherwise it takes the names from its cache, sums the record's thread and drop counts into `LiveCounts`, reads the run from the channel, calls `make_closed_period` and publishes. Run capping keeps runs with `end - begin >= min_run_ns`, then the `max_runs` longest, then sorts by (thread, lane, begin).

**Tests:**
- `Small_NameSnapshotCache.ReusesTheSnapshotWhileTablesAreUnchanged`: same pointer twice.
- `Small_NameSnapshotCache.RebuildsWhenANameASiteATrackOrAThreadNameGrew`: four cases, four new pointers.
- `Small_NameSnapshotCache.ASnapshotKeepsItsTextAfterTheRecordGrows`.
- `Small_ClosedPeriod.KeepsTheLongestRunsAndCountsTheOmitted`; `DropsRunsBelowTheMinimumLength`; `KeepsRunsInThreadLaneBeginOrder`; `ZeroMaxRunsKeepsNone`; `AWarmupPeriodHasNoRuns`; `LeavesTheSourcePeriodUntouched`; `CarriesItsRunAndSequence`.
- `Small_LiveChannel.LatestIsNullBeforeAPublication`; `HistoryReturnsTheLastNOldestFirst`; `TheRingOverwritesItsOldestWhenFull`; `AReaderKeepsAnOverwrittenPeriodAlive`: hold a period, publish three times the capacity, its fields and `names` are intact and a `weak_ptr` to it is not expired; `WatchedFollowsLiveViewCopiesAndMoves`; `BeginRunClearsTheHistoryAndBumpsTheRun`.
- `Medium_LiveChannel.ReadersNeverSeeSequenceGoBackwards`: four readers against 10 000 publishes; `latest()` sequence never decreases per reader; every `history(50)` is consecutive; `latest()` read after `history(n)` is not behind `history(n).back()`.
- `Small_LiveView.ADefaultViewIsInvalidAndReadsNothing`; `AViewOutlivesItsChannelsOwner`.
- `Small_PeriodPublisher.PublishesNothingWhileTheChannelIsNotWatched`; `PublishesWithConsecutiveSequencesAndTheChannelsRun`; `SharesNamesUntilATableGrows`; `SumsTheRecordsLateAndDroppedCounts`.

**Build lines:**
- `modules/session/live/CMakeLists.txt`: `sushiprofiler_add_module(NAME live TIER session SOURCES name_snapshot.cpp closed_period.cpp live_channel.cpp live_view.cpp period_publisher.cpp DEPENDS version record)`
- `tests/session/live/CMakeLists.txt`: `sushiprofiler_add_module_tests(MODULE live SOURCES test_name_snapshot.cpp test_closed_period.cpp test_live_channel.cpp test_live_view.cpp test_period_publisher.cpp)`
- `cmake/SushiProfilerTiers.cmake`: in `SUSHIPROFILER_MODULE_TIERS` add `live session` after `fold session` and before `registry session`; `live` has no same-tier edge (its dependencies are lower tiers).

**Docs and changelog:** the module README; `docs/guides/LIVE_VIEW.md` (how a frame loop reads `latest()`, the copy rule, `history`, names, `break_down_period`, the lag a view has and the keys that set it, that `latest()` takes a short library lock), linked from `docs/README.md` under Guides. `- 2026-10-26 — live: Added ClosedPeriod, NameSnapshot, LiveChannel, LiveView and PeriodPublisher, the immutable closed periods a tool reads while a session runs (modules/session/live/, docs/guides/LIVE_VIEW.md).`

**Closes:** half of RW-14 (the reader side); L13 closes it.

**Acceptance:** all tests above pass, including the reader-thread test.

---

### L9: Retention, bounded assembler state, and the assembler's collaborators

**Files:**
- Create: `modules/session/consumer/include/SushiProfiler/consumer/period_retention.hpp`, `modules/session/consumer/source/period_retention.cpp`
- Modify: `period_assembler.hpp` (`AssemblySettings` loses `window`; gains `keep_periods`; new `AssemblyParts`), `period_assembler.cpp` (`close`, `copy_tables`, `finish`, `place`), `modules/session/consumer/README.md`
- Create: `tests/session/consumer/assembly_rig.hpp` (a header-only test helper that owns a `Window`, `PauseLog`, `LiveChannel`, `PeriodPublisher` and `StreamWriterSet` and returns `AssemblyParts`)
- Modify: `tests/session/consumer/test_period_assembler.cpp`, `test_consumer.cpp`, `benchmark_consumer.cpp` (they build the assembler with the new constructor)
- Test: `tests/session/consumer/test_period_retention.cpp`

**Interfaces produced:**
```cpp
// AssemblySettings: values only
NameID timeline = 0U; NameID run_timeline = 0U; std::uint64_t linger_ns = 100'000'000U;
CalibrationPoint origin; std::optional<std::uint32_t> keep_periods;   // absent keeps all

// the collaborators, references that outlive the assembler
struct AssemblyParts
{
    const Window& window;        // consulted only when a timeline is followed
    PauseLog& pauses;
    PeriodPublisher& publisher;
    StreamWriterSet& writers;
};
PeriodAssembler(const AssemblySettings& settings, RunRecord& record, const NameTable& names,
                const SiteRegistry& sites, std::vector<Source*> sources,
                const AssemblyParts& parts);
void end_writers();     // ends every begun stream writer once; problems join problems()

// period_retention.hpp
void trim_periods(std::vector<PeriodRecord>& periods, std::optional<std::uint32_t> keep,
                  bool final_trim);
```
`trim_periods`: absent does nothing; 0 clears; otherwise it trims to `keep` when `final_trim`, and when `periods.size() >= 2 * keep` otherwise. `close` appends, then publishes, then trims with `false`; `finish` trims with `true`. With `keep` 0 the period is written to the stream writers and published, then not retained. `copy_tables` appends only the names and sites added since the last call. The closed-span list coalesces adjacent spans, stays sorted, is searched by binary search and holds at most 4096 entries, dropping the oldest. The writer set, pause log and publisher come from L10 to L12, so L9 to L12 land in one build and the module compiles only with all four.

**Tests:**
- `Small_PeriodRetention.AbsentKeepsEverything`; `ZeroClearsAtOnce`; `TrimsToNWhenTheListReachesTwoN`; `FinalTrimLeavesExactlyN`; `KeepsTheNewestPeriodsInOrder`.
- `Small_PeriodAssembler.KeepsTheLastNPeriodsAtStop`; `KeepsNoPeriodsWhenZeroButStillStreamsThem`; `KeepsEveryPeriodWhenAbsent`.
- `Small_PeriodAssembler.ALateEventOfATrimmedPeriodIsStillCountedLate`: retention does not forget closed spans.
- `Small_PeriodAssembler.ALateEventOlderThanTheTrackedSpansCountsAsOutside`: more than 4096 windowed cycles, then an event in the first.
- `Small_PeriodAssembler.TheRecordsTablesGrowWithoutRecopying`: ids stay valid across closes after the tables grow.
- Every existing assembler, consumer and benchmark test builds through the rig and is otherwise unchanged.

**Build lines:** `modules/session/consumer/CMakeLists.txt` SOURCES `period_retention.cpp`; `tests/session/consumer/CMakeLists.txt` SOURCES `test_period_retention.cpp`.

**Docs and changelog:** consumer README step 5 and the retention paragraph. `- 2026-10-26 — consumer: Added period retention, bounded the assembler's closed-span list and made the tables grow instead of being recopied each period; PeriodAssembler takes its collaborators as AssemblyParts (modules/session/consumer/).` The constructor change breaks a program that builds an assembler itself; the entry says so.

**Acceptance:** the retention tests pass and every existing assembler test, rebuilt on the rig, is unchanged.

---

### L10: Pause log

**Files:**
- Create: `modules/session/consumer/include/SushiProfiler/consumer/pause_log.hpp`, `.../source/pause_log.cpp`
- Modify: `period_assembler.cpp` (`add`, `queue`, `close`), README
- Test: `tests/session/consumer/test_pause_log.cpp`, `test_period_assembler.cpp`

**Interfaces produced:**
```cpp
inline constexpr const char* K_PAUSED_FACT = "paused";

class SUSHIPROFILER_API PauseLog final      // thread-safe
{
    public:
        static constexpr std::size_t K_MAX_INTERVALS = 4096U;   // the oldest closed one is dropped
        void begin(std::uint64_t steady_ns);    // ignored while an interval is open
        void end(std::uint64_t steady_ns);      // ignored with none open
        [[nodiscard]] bool paused() const;      // an interval is open
        [[nodiscard]] bool overlaps(std::uint64_t begin_steady_ns,
                                    std::uint64_t end_steady_ns) const;
        [[nodiscard]] std::vector<std::uint64_t> ends_after(std::uint64_t steady_ns) const;
        void forget_before(std::uint64_t steady_ns);   // drops closed intervals ending before it
};
```
`overlaps` is true when `begin < interval end && end > interval begin`; an open interval ends at the maximum. `ends_after` returns the end of every closed interval later than the argument. Callers order their calls so the log is never behind the gate: `begin` before `Collector::hold`, `end` before `Collector::release` (D19). The assembler fetches `ends_after` once per `add`, converts each end to record time and appends it to `reopened_ns_`, so `queue` resets a thread's lanes across a pause exactly as it does across a window gap (D19). `close` converts the period to steady time with `settings_.origin.steady_ns` and adds `Fact{K_PAUSED_FACT, "true"}` when it overlaps. Every close applies D24: it drops `reopened_ns_` entries at or below the smallest `last_write_ns` of the threads that have not retired (the assembler keeps a `retired` flag per thread from `ThreadDrain::retired`) and calls `forget_before` with the smaller of that time and the oldest open period's begin, in steady time. Sources are still asked to collect a paused period; its zones are empty because the gate was closed.

**Tests:**
- `Small_PauseLog.OverlapsAPeriodThatCrossesTheStart`; `OverlapsAPeriodThatCrossesTheEnd`; `ABoundaryTouchIsNotAnOverlap`; `AnOpenIntervalOverlapsEverythingAfterItsStart`; `BeginIsIdempotentAndEndWithoutBeginIsIgnored`; `ForgetsClosedIntervalsBeforeATime`; `EndsAfterReturnsOnlyLaterEnds`; `DropsTheOldestIntervalPastTheCap`.
- `Medium_PauseLog.ConcurrentBeginEndAndOverlapsAgree`: one writer toggling 10 000 times, two readers; no crash and no overlap answer for a range the writer never covered.
- `Small_PeriodAssembler.MarksAPeriodThatOverlapsAPauseAndNoOther`; `APausedPeriodKeepsItsMarksAndHasNoZones`; `ASourceStillCollectsAPausedPeriod`.
- `Small_PeriodAssembler.ALaneOpenAcrossAPauseIsResetAfterResume`: a lane begins, a pause ends, events after the end start in no lane and the lane's late end counts as unmatched, not as an open lane at stop.
- `Small_PeriodAssembler.OldPauseIntervalsAreForgottenOnceEveryLiveThreadWroteLater`, `ALiveThreadThatWroteEarlierKeepsAPauseInterval` and `ARetiredThreadDoesNotHoldBackForgettingAPause`: the pause log, which the rig exposes, shows what the pruning kept.

**Build lines:** `modules/session/consumer/CMakeLists.txt` SOURCES `pause_log.cpp`; `tests/session/consumer/CMakeLists.txt` SOURCES `test_pause_log.cpp`.

**Docs and changelog:** README (the `paused` fact and its meaning: a period overlapped a pause, so its zones are partial). `- 2026-10-26 — consumer: Added PauseLog and the paused fact on periods that overlap a pause (modules/session/consumer/).`

**Acceptance:** the pause log and assembler tests above pass.

---

### L11: The stream writer set

**Files:**
- Create: `modules/session/consumer/include/SushiProfiler/consumer/stream_writer_set.hpp`, `.../source/stream_writer_set.cpp`
- Modify: `period_assembler.cpp` (writes and `end_writers` go through the set; `NamedStreamWriter` is removed), README, `tests/session/consumer/assembly_rig.hpp`
- Test: `tests/session/consumer/test_stream_writer_set.cpp`

**Interfaces produced:**
```cpp
class SUSHIPROFILER_API StreamWriterSet final
{
    public:
        struct Entry { std::string name; StreamWriter* writer = nullptr; bool enabled = true; };
        explicit StreamWriterSet(std::vector<Entry> entries);
        [[nodiscard]] bool knows(std::string_view name) const;
        [[nodiscard]] bool request(std::string_view name, bool enabled);  // any thread; false if unknown
        [[nodiscard]] std::string begin_enabled(const RunRecord& record);  // empty or the first failure
        void apply_requests(const RunRecord& record, std::vector<std::string>& problems);
        void write_period(const RunRecord& record, const PeriodRecord& period,
                          const std::vector<ZoneRun>& runs, std::vector<std::string>& problems);
        void end_all(const RunRecord& record, std::vector<std::string>& problems);
};
```
Requests queue under a mutex held for the queue swap only; the writer list is touched by the consumer thread alone. `apply_requests` runs at the start of `PeriodAssembler::close`: an enabled writer that has not begun is begun, and a failure is a problem and `Failed`. `write_period` skips disabled, unbegun and failed writers; a failing one becomes `Failed` and is named in the problems as today. `end_all` ends each begun writer once. `Session` begins the writers enabled at start through `begin_enabled`.

**Tests:**
- `Small_StreamWriterSet.ADisabledWriterIsNotBegunUntilEnabled`; `ARequestAppliesOnlyInApplyRequests`; `TurningOffStopsPeriodsButKeepsTheOutputOpenUntilEnd`; `AWriterThatFailsToBeginWhenEnabledIsStoppedAndNamed`; `AFailedWriterIgnoresLaterRequests`; `EndAllEndsEveryBegunWriterOnceAndNoOther`; `AnUnknownNameIsRefused`; `BeginEnabledStopsAtTheFirstFailure`.
- `Medium_StreamWriterSet.RequestsFromAnotherThreadNeverTearAPeriod`: a thread toggles 1000 times while 1000 periods are written; a recording writer asserts the order begin, writes with ascending indices, end, and no write before begin or after end.
- `Small_PeriodAssembler.AWriterEnabledBeforeACloseReceivesThatPeriod`; `AWriterDisabledBeforeACloseDoesNotReceiveIt`; `ATogglePostedWhileAPeriodIsOpenTakesEffectAtTheNextClose`; the existing failing-writer test runs on the set.

**Build lines:** `modules/session/consumer/CMakeLists.txt` SOURCES `stream_writer_set.cpp`; `tests/session/consumer/CMakeLists.txt` SOURCES `test_stream_writer_set.cpp`.

**Docs and changelog:** consumer README and `stream_writers` README (a writer may start disabled). `- 2026-10-26 — consumer: Added StreamWriterSet, which switches stream writers on and off at a period boundary (modules/session/consumer/).`

**Acceptance:** the twelve tests pass.

---

### L12: Publication and the poll ceiling

**Files:**
- Modify: `period_assembler.cpp` (`close` calls `parts_.publisher.closed(...)` after the append and before the trim, D22), `consumer.hpp` and `consumer.cpp`, README
- Modify: `modules/session/consumer/CMakeLists.txt` (DEPENDS)
- Test: `tests/session/consumer/test_period_assembler.cpp`, `tests/session/consumer/test_consumer.cpp`

**Interfaces produced:**
```cpp
// consumer.hpp
inline constexpr std::chrono::milliseconds K_DEFAULT_LIVE_POLL{4};
[[nodiscard]] SUSHIPROFILER_API std::chrono::milliseconds next_poll(
    std::chrono::milliseconds current, double fullest, std::chrono::milliseconds slowest) noexcept;
Consumer(Collector& collector, PeriodAssembler& assembler) noexcept;            // unchanged
Consumer(Collector& collector, PeriodAssembler& assembler, const LiveChannel& live,
         std::chrono::milliseconds live_slowest) noexcept;
void wake();   // ends the current wait early, so a new viewer is served at once
```
The old `next_poll(current, fullest)` stays and calls the new one with `K_SLOWEST_POLL`. While `live.watched()` the loop uses `live_slowest` as the ceiling; `wake()` sets a flag under `mutex_` and notifies, and the loop treats it as a timeout. The assembler holds no pointer to the channel: publication is `parts_.publisher.closed(record_.periods.back(), record_)`.

**Tests:**
- `Small_NextPoll.CeilingLimitsTheDoubling`; `TheFastestPollStillWinsWhenARingIsFull`.
- `Medium_Consumer.AWatchedConsumerDrainsAtTheLiveCeiling`: with a `LiveView` attached the drain count reaches 25 within 5 s of waiting on a condition.
- `Medium_Consumer.WakeShortensAnIdleSleep`: let the wait grow, attach, `wake()`, a drain happens within 5 s on a condition.
- `Small_PeriodAssembler.PublishesEachClosedPeriodWhenAViewIsWatching`; `PublishesNothingWithoutAView`; `TheClosedPeriodCapsItsRunsButTheRecordKeepsThemAll`; `PublishesInOrderWithConsecutiveSequences`; `AClosedPeriodOutlivesTheAssembler`; `PublishesAfterTheWritersAndTheRecord`: a writer asserts the channel still shows the previous period while it writes, and the record holds the period when `latest()` first shows it.
- `Small_PeriodAssembler.KeepPeriodsZeroStillPublishesEachPeriod`: `keep_periods` 0 leaves the record empty and the channel holds every period (D22).

**Build lines:** `modules/session/consumer/CMakeLists.txt` DEPENDS adds `live`; `cmake/SushiProfilerTiers.cmake` SAME_TIER_EDGES adds `consumer live`.

**Docs and changelog:** consumer README (publication, poll ceiling, `wake`). `- 2026-10-26 — consumer: The assembler publishes each closed period through a PeriodPublisher while a view exists, and the consumer polls at most every live.max_poll_ms then (modules/session/consumer/).`

**Acceptance:** the ten tests pass.

---

### L13: The `Session` surface

**Files:**
- Modify: `modules/session/session/include/SushiProfiler/session/session.hpp`, `modules/session/session/source/session.cpp`, `modules/session/session/README.md`
- Modify: `modules/capture/periods/include/SushiProfiler/periods/period_timeline.hpp` and `source/period_timeline.cpp` (`reset()`, which D21 needs), `tests/capture/periods/test_period_timeline.cpp`
- Modify: `docs/architecture/ARCHITECTURE.md` (session path), `docs/guides/LIVE_VIEW.md`, `docs/reference/CONFIG_FORMAT.md` (behaviour notes)
- Test: `tests/session/session/test_session.cpp`

**Interfaces produced:**
```cpp
enum class WriterSwitch : std::uint8_t { Queued, UnknownWriter, NotStreaming, NotRecording };

// PeriodTimeline
void reset() noexcept;      // the next mark opens period 0 again; current() is K_NO_PERIOD

// Session (one mutex, control_mutex_, serialises these against each other)
[[nodiscard]] LiveView live();                      // wakes the consumer
[[nodiscard]] bool pause();                         // PauseLog::begin then Collector::hold
[[nodiscard]] bool resume();                        // PauseLog::end then Collector::release
[[nodiscard]] WriterSwitch set_writer_enabled(std::string_view name, bool enabled);
[[nodiscard]] std::string restart();                // next run on this object; empty or the refusal
```
`restart` is allowed only after `stop()` and refuses otherwise with a message. It destroys the previous run's consumer, assembler, publisher, writer set, pause log and window, clears the sources, writers and record, resets every timeline, calls `LiveChannel::begin_run()` and runs `open()` again. The static `start` is unchanged and builds the first run the same way. The `Session` creates its `LiveChannel` (sized `live.history`) in its constructor and keeps it for its whole life, so a `LiveView` taken before `stop` reads the last periods of the stopped run and the next run's after `restart` (V3).

`open` and `begin_recording` now: build one `StreamWriterSet` (begin only the writers enabled at start through `begin_enabled`; a failure is a start error as today), one `PauseLog`, one `PeriodPublisher` over the channel with `RunCap{live.max_runs, live.min_run_ns}`, and a `Window` (the configured schedule when a timeline is followed, else one with `active_all`); call `Collector::want_sources(true)` after the last source's `begin` succeeded, `false` in `pause`, `true` in `resume`, and `false` at the start of `shut_down`, before the first source's `end` (D31); read each started source's `max_delay_ns`, refuse one above 60 000 ms naming the source, and set `linger_ns` to the larger of `buffers.linger_ms` and every delay; pass `keep_periods` and the parts to the assembler and the channel and `live.max_poll_ms` to the `Consumer`. `shut_down` stops the consumer, ends the sources, calls `assembler_->end_writers()`, then collects `problems()`, and clears the slot as before so a second run works (RW-20). `state_facts` writes `config.window` with `active all` when `active_all`, and `config.record.keep_periods` (D27). `stop` returns the record with the retained periods.

**Tests** (`tests/session/session/test_session.cpp`):
- `Medium_Session.LiveViewShowsEachClosedPeriodWhileTheSessionRuns`: 60 periods; `latest()` sequence rises; `history(10)` is 10 consecutive periods; `NameSnapshot` resolves every site in them.
- `Medium_Session.ALiveViewOutlivesTheSessionAndKeepsItsLastPeriods`.
- `Medium_Session.AViewTakenBeforeStopKeepsWorkingAfterRestart`: the view read after `restart` shows run 2, sequence from 0, and none of run 1.
- `Medium_Session.KeepsTheLastNPeriodsInTheStoppedRecord`: keep 5 of 30 gives indices 25 to 29.
- `Medium_Session.KeepPeriodsZeroReturnsNoPeriodsButLiveStillPublishes`.
- `Medium_Session.AWriterStartedDisabledWritesNothingUntilEnabledAtABoundary`: the trace file does not exist until the switch; periods before the switch are not in it, later ones are.
- `Medium_Session.RefusesAWriterToggleForAnUnknownOrRecordWriterName`: `UnknownWriter`, `NotStreaming`, and `NotRecording` after stop.
- `Medium_SessionPause.PauseMarksTheCrossedPeriodsAndResumeRecordsAgain`: periods overlapping the pause carry `paused`, others do not; a zone inside the pause is absent; indices are continuous; periods after `resume` have zones.
- `Medium_SessionPause.PauseAndResumeReportWhetherTheyTookEffect`.
- `Medium_Session.ASourceDelayHoldsPeriodsOpenAndLosesNone`: a fake source with a 50 ms delay hands over spans three periods late; every span is in its period and the periods close in order.
- `Small_Session.RefusesASourceDelayAboveTheLimit`: the error names the source.
- `Medium_Session.EveryPeriodWindowRecordsAllAfterWarmup`: `active: "all"`, 200 periods after 3 warm-up, none lost (RW-19).
- `Medium_Session.ARestartAfterStopReadsASecondRecord`: two runs on one object give two records, the second with no event of the first and periods numbered from 0 (RW-20); a second static `start` after the first session is destroyed works too.
- `Medium_SessionPause.SourcesAreNotWantedWhilePaused`: the fake source records the flag in `begin`, `collect` and `end`; it is false in `begin` and before `start`, true in `collect` while running, false after `pause`, true after `resume`, and false in `end` and after `stop`.
- `Medium_Session.ASourceFactReachesTheStoppedRecordAndItsFile`: the fact survives the record writer and `read_record` (RW-7).
- `Medium_Session.ControlCallsAreSerialisedAgainstStopAndRestart`: one thread toggles writers, pauses and resumes and takes views in a loop while another stops and restarts the session; no crash, every return value is one the contract names.
- `Small_Session.RecordsKeepPeriodsInItsFacts`: `config.record.keep_periods` is `all`, `0` or `N`.
- `Small_PeriodTimeline.ResetStartsPeriodZeroAgain` (in `tests/capture/periods`).

**Build lines:** `modules/session/session/CMakeLists.txt` DEPENDS adds `live`; `cmake/SushiProfilerTiers.cmake` SAME_TIER_EDGES adds `session live`.

**Docs and changelog:** session README; `docs/architecture/ARCHITECTURE.md` session path; the guide's final form. Entries: `- 2026-10-26 — session: Added live, pause, resume, set_writer_enabled and restart, source delays and retention to Session (modules/session/session/).` `- 2026-10-26 — periods: Added PeriodTimeline::reset (modules/capture/periods/).` `- 2026-10-26 — session: A restart after a stop in one process is stated and tested (modules/session/session/).`

**Closes:** RW-14, RW-20, and with L6 and L7, RW-19 and RW-7.

**Acceptance:** the eighteen Session tests and the timeline test pass, and the full existing session suite is unchanged and green.

---

### L14: The lag benchmark and the budget check (orchestrator runs it at the final build)

**Files:**
- Create: `tests/session/session/benchmark_live_lag.cpp`
- Modify: `tests/session/session/README.md`

**Scenario.** A session follows `bench.frame` with `buffers.linger_ms` 0 and a `LiveView` held. A marking thread marks 120 periods at 60 Hz on an absolute schedule (`sleep_until`), each with a small zone. A poller thread reads `latest()` every 100 microseconds and records the steady time at which each `sequence` first shows. The lag of a period is that time less the period's end (`origin_steady_ns + period.end_ns`). Two cases, each run five times: `linger0` (no source) and `delay50` (a fake source with `max_delay_ns` of 50 ms that delivers a span 3 periods late). The poller's 100 microsecond step is part of the number and the output says so.

**Output** (no verdict, never fails): `build_type`, `periods 120`, `runs 5`, `live_max_poll_ms`, then per case `live_lag_<case>_median <ms> ms`, `..._p95`, `..._max`, and `drains <n>`.

**Budget check.** At the same build the orchestrator runs the existing zone benchmark on the `v0.2.0` tag and on this branch, same machine, five runs each, and pastes both. The rows to hold: gate closed under 2 ns, recording under 25 ns, no allocation after `register_this_thread` (`Small_Zone` allocation tests green).

**Build lines:** `tests/session/session/CMakeLists.txt`: add `BENCHMARKS benchmark_live_lag.cpp` to the `sushiprofiler_add_module_tests` call.

**Docs and changelog:** the measured numbers go in `REPORT.md` and, at release, in `docs/design/PROFILER/README.md` beside the performance budget. `- 2026-10-26 — session: Added benchmark_live_lag, which measures the time from a period's end to its publication (tests/session/session/).`

**Acceptance:** both cases print five runs with spread, and the budget rows are unchanged within their noise or the difference is reported.

---

### L15: Release v0.3.0 (orchestrator)

**Files:** `CMakeLists.txt` (`project(... VERSION 0.3.0)`), `cmake/SushiProfilerLibrary.cmake` (SOVERSION 0.3), `docs/reference/CHANGELOG.md`, `docs/design/PROFILER/README.md`, `docs/design/PROFILER/CAPTURE_AND_SESSION.md`, `docs/design/REMAINING_WORK.md`, `docs/README.md`, root `README.md` Status, this work folder's status line.

**Steps:**
1. Write the decisions of D1 to D18 that are behaviour into `CAPTURE_AND_SESSION.md`: the session surface, the window's `"all"`, the two gate reasons, the live view and its lag, retention, the delay rule, named counters; update its "The session" signatures and its configuration example. Mark P2c built with the date in the phase table and the status line; `docs/README.md` says P1, P2 and P2c are built.
2. Delete RW-7, RW-13, RW-14, RW-19 and RW-20 from `REMAINING_WORK.md`; `Next id` stays RW-23; remove any of them from `## Next`.
3. Run `git grep -n 'consumer.linger_ms' -- '*.md'` and every old name an entry changed; correct each hit.
4. One build and the full test run through the project CLI, then every checker under `tools/`, then a reviewer who did not write the manual reads it against the code (`versioning-and-release`, "Cutting a release").
5. Rename `## Unreleased` to `## v0.3.0 — <date>`; open an empty `## Unreleased`; commit `chore(release): v0.3.0`; tag annotated; push only on the owner's word.

**Acceptance:** all checkers clean, all tests green, `REPORT.md` holds the L14 numbers, the tag exists and nothing is pushed without approval.
