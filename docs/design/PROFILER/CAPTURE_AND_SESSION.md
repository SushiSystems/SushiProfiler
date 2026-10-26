# Capture and session

## Modules

A repository shaped like SushiLog: `modules/`, the `sp` CLI, the documentation tree, a hub
manifest, pinned by tag from every library that uses it. It depends on the C++17 standard library and
Threads alone, so a program outside the stack takes it without taking anything else. It logs
nothing: every failure returns a status, and drops are counted in the record. Namespace
`SushiProfiler`.

| Tier | Module | Holds |
| --- | --- | --- |
| foundation | `clock` | `TraceClock`, `CalibrationPoint`, `Calibration` |
| foundation | `events` | `TraceEvent`, `DeviceSpan`, `Counter`, `AttributeBlock`, ids |
| capture | `sites` | `ZoneSite`, the site registry, the per-site enable bytes, name interning |
| capture | `rings` | `ThreadRing`, thread registration and retirement |
| capture | `collector` | The process-wide gate and the drainer handoff (see "The hot path" below) |
| capture | `zones` | `Zone`, `SP_ZONE`, `SP_ZONE_NAMED`, `SP_COUNTER`, `SP_THREAD_NAME` |
| capture | `lock_wait` | `ContendedLock` and `SP_ZONE_LOCK_WAIT`: a lock that opens a zone only when it waits |
| capture | `periods` | `PeriodTimeline` and `mark_period` |
| record | `record` | `RunRecord` and its JSON reader and writer |
| record | `statistics` | Steady-state check, median, percentiles, bootstrap interval |
| record | `analysis` | The `Analysis` interface, the generic analyses ([RECORD_AND_ANALYSES.md](RECORD_AND_ANALYSES.md)) and `AnalysisRunner`, which runs the named analyses over a record |
| record | `record_writers` | The `RecordWriter` interface, given a finished record: `terminal`, `record` |
| record | `compare` | Two or more records side by side |
| session | `config` | `ProfilerConfig` and its JSON reader |
| session | `registry` | `Registry`: factories for sources, analyses, writers and environment contributors, by name |
| session | `window` | The capture schedule over one timeline's periods |
| session | `sources` | The `Source` and `SpanSink` interfaces |
| session | `environment` | The `EnvironmentContributor` interface and the host facts SushiProfiler reads itself (OS, CPU, threads) |
| session | `fold` | Events to one zone tree per thread per period |
| session | `stream_writers` | The `StreamWriter` interface, given each period as it closes: `chrome_trace` |
| session | `consumer` | The consumer thread: drains, folds, collects from sources, appends to the record, feeds stream writers |
| session | `session` | `Session`: starts and stops one consumer with the parts the configuration names |
| tools | `sp_tool` | The program behind `sp show` and `sp compare` |

The record tier does not depend on the session tier, so `sp show` analyses a file with no session
running. A lower tier never includes a higher one; the layering checker the other repositories run
holds.

Attribute keys are a contract between the library that writes them and the analysis that reads
them. Each library declares its keys as constants in one header and lists them on one page,
`docs/reference/PROFILER_KEYS.md`; the record stores the key names, so a renamed key shows as
missing, not as a wrong number.

## Events

```cpp
using SiteID = std::uint32_t;   // 0 is no site
using NameID = std::uint32_t;   // 0 is the site's own name
using TrackID = std::uint32_t;

struct TraceEvent               // 24 bytes, trivially copyable
{
    std::uint64_t begin_ticks;
    std::uint64_t end_ticks;    // equal to begin_ticks for a marker or a counter
    SiteID site;                // high 4 bits: kind (zone, marker, counter, period)
    NameID name;                // a counter stores its value in end_ticks as the bits of a double
};

struct DeviceSpan               // written by a source, never on a hot path
{
    std::uint64_t queued_ns;    // submit time, converted by the source onto the steady clock
    std::uint64_t start_ns;     // likewise
    std::uint64_t end_ns;       // likewise
    std::uint64_t period;       // the period the work belongs to
    std::uint32_t uncertainty_ns; // the source's clock-conversion uncertainty
    TrackID track;
    NameID name;
    std::uint32_t submit_thread;
    std::uint32_t attributes;   // offset into the period's attribute arena, or none
};
```

A device clock is converted by its source, which knows the pairing; SushiRuntime's carries the
pairing's `uncertainty_ns` into each span. `device_idle` treats a gap shorter than the uncertainty
of its two spans as unattributed rather than guessing.

An attribute block is a run of `(KeyID, kind, value)` triples in a per-period arena: integers,
doubles and interned strings. SushiBLAS writes `flops`, `bytes_read`, `bytes_written`, `dtype`;
SushiRuntime writes `node`, `operation`, `launched_width`, `live_width`; SushiAI writes `ir_node`
and `op`. SushiProfiler reads no key by name except in the analyses that say so.

## The hot path

```cpp
#define SP_ZONE(name_literal) \
    static const ::SushiProfiler::ZoneSite SP_UNIQUE(site){name_literal, __func__, __FILE__, \
                                                         __LINE__, nullptr}; \
    ::SushiProfiler::Zone SP_UNIQUE(zone)(SP_UNIQUE(site))
```

- `Zone`'s constructor loads the gate (`std::atomic<std::uint32_t>`, relaxed). Zero means off: the
  zone stores nothing and its destructor tests one member.
- While on, it reads the site's id, registering the site on first use under a mutex, then the
  site's enable byte. If enabled, it reads the clock once; the destructor reads it again and pushes
  one `TraceEvent`.
- The push is the engine's single-producer ring write with its cached tail. A full ring counts a
  drop. The producer never notifies anyone, because a notification is a system call and a
  real-time thread may not make one; the consumer polls instead (see "The session").
- A thread's ring is created when the thread first records, or ahead of time through
  `register_this_thread(capacity)` so the first zone of a hot thread does not allocate.
- At level 0 the macros expand to nothing. Level 1 is stage zones, level 2 fine zones, level 3
  per-element detail. The runtime level in the configuration can lower what the compiled level
  allows, never raise it.

## Records other than zones

The surveys of SushiDSP and SushiTrack on 2026-10-26 found measurements that are not a time range
on one thread. Each is a kind of `TraceEvent`, written through the same ring with the same cost
rules, and none carries a client's vocabulary.

| Kind | Macro or call | Event fields | Serves |
| --- | --- | --- | --- |
| Counter | `SP_COUNTER(site, value)`; the site declares a unit and an operation: `add`, `set`, `max`, `min` | time, value as the bits of a double | Counts per period (objects in and out, solver iterations); a worst case is `max`, not a sum |
| Value | `SP_VALUE(site, value)`; the site declares a unit | time, value | Samples that are not times (a level in dB, a queue depth); every sample kept, so a distribution is possible |
| Instant | `SP_INSTANT(site)` | time | An occurrence: a buffer underrun a device reports, a tracker losing an object |
| External time | `record_instant_at(site, steady_ns)`, `record_zone_at(site, begin_ns, end_ns)` | caller's times on the steady clock | A clock outside the profiler: a device callback, another library's own timer |
| Flow | `SP_FLOW_BEGIN(site, id)`, `SP_FLOW_STEP(site, id)`, `SP_FLOW_END(site, id)` | time, the caller's 64-bit id | Latency of one item across threads: a parameter handed to a real-time thread, a frame through a pipeline. Written as Chrome flow events |

When one thread serves several independent streams, `SP_LANE(lane_id)` opens a lane scope: it
writes a lane-begin and a lane-end event, and every event between them belongs to that lane. The
record and the Chrome trace show each lane as its own row under the thread. A lane id is interned
like a name.

A counter or value site is a static object, registered like a zone site. Real-time rules hold for
every kind: no allocation after `register_this_thread`, no lock, no system call, a bounded cost, a
full ring drops and counts. The clock read is a user-space instruction on both platforms; the
record states the clock and its measured cost per read.

## Periods and the window

A timeline is a named sequence of periods: `"train.step"`, `"bench.iteration"`, `"engine.frame"`.
`PeriodTimeline::mark()` returns the new period's index and pushes a period event carrying it on
the caller's ring. The index advances while the gate is closed, so a recorded mark and a source
that stamps its data with the index agree. Several timelines may run at once; the configuration names the one the window follows.

A timeline may carry a budget: `PeriodTimeline(name, budget_ns)`, or `mark(budget_ns)` when it
changes per period. A real-time block's budget is its duration; a frame's is its interval. The
`budget` analysis reports each period's used fraction, the count over budget and the worst period.

A period may carry tags, `timeline.tag(key, value)`, written once per period off the hot path:
the input sequence, the variant under test. The record keeps them and `sp compare` groups by them.

Because a closed gate writes no mark, the window cannot learn of a period from the record. P2
gives `PeriodTimeline` an observer, an interface declared in the capture tier and implemented by
the window, which each `mark()` calls with the index it opens; the window opens or closes the gate
there, before the mark is written.

The window is PyTorch's schedule: `skip` periods are ignored, then cycles of `wait` (nothing
recorded), `warmup` (recorded, marked warm-up), `active` (recorded), repeated `repeat` times, 0
meaning until the session stops. The gate opens at the first warm-up period and closes after the
last active one, so periods outside the window cost what an off profiler costs.

## The session

```cpp
class Session final
{
    public:
        static Result<std::unique_ptr<Session>> start(const ProfilerConfig& config,
                                                       const Registry& registry);
        PeriodTimeline& timeline(std::string_view name);
        Result<RunRecord> stop();                 // joins the consumer; returns the record
};
```

`stop` returns the record and nothing else. The host then calls
`AnalysisRunner::run(record, config.analyses)` and each `RecordWriter`; `sp show` takes the same
path from a file. Stream writers are the only writers that run inside the session.

- One session runs per process; a second `start` is refused with a status naming the first.
- `start` builds the sources, analyses and writers the configuration names from the registry,
  refusing an unknown name with the list of known ones.
- The consumer thread drains rings, folds each closed period into one tree per thread with the
  engine's fold arithmetic on 64-bit offsets, asks each source for that period's spans and
  counters, and appends the period to the record. Writers that stream (the Chrome trace) write
  each period as it closes, so memory holds one window, not the run.
- The consumer polls `Collector::drain` at an interval it adapts from each ring's fill: 1 ms while a
  ring is over half full, doubling after each quiet drain up to 1 s, so an idle session wakes
  under once a second and a busy one drains before a ring fills.
- Dependencies arrive through `start`; the only process-wide state is the collector, because a
  macro at any call site must reach it.

## Sources

```cpp
class Source
{
    public:
        virtual ~Source() = default;
        [[nodiscard]] virtual std::string_view name() const noexcept = 0;
        virtual Status begin(SourceContext& context) = 0;   // tracks, keys, clock
        virtual void collect(std::uint64_t period, SpanSink& sink) = 0;
        virtual void end() = 0;
};
```

A source that receives its data on another thread, as SushiRuntime's does after each run, queues
it and hands it over in `collect`. `collect` runs on the consumer thread.

A span's period is the index of the configured timeline's current period when the work was
submitted: the source reads `PeriodTimeline::current()` (one relaxed load) when it is handed a
run, so the two runs of one anchor-free step both land in that step. `collect(period)` returns
every span tagged with that period; a span tagged with a later period waits for it. A repeated
dispatch of one node keeps its `invocation` as an attribute, so no two dispatches collapse.

An `EnvironmentContributor` adds named facts to the record's `environment` when the session
starts: SushiRuntime's adds the device, driver and backend, and each library adds its own name,
tag and commit.

## Configuration

`ProfilerConfig` is one value every host fills from its own settings. Its JSON form:

```json
{
    "sushiprofiler": 1,
    "enabled": true,
    "level": 2,
    "groups": ["Data", "Device", "Host stage"],
    "timeline": "train.step",
    "window": { "skip": 0, "wait": 0, "warmup": 3, "active": 20, "repeat": 1 },
    "sources": { "sushiruntime.device": {}, "sushiai.attribution": {} },
    "analyses": ["breakdown", "device_idle", "top_spans", "roofline"],
    "writers": {
        "terminal": { "top": 20 },
        "record": { "path": "run.sprec.json" },
        "chrome_trace": { "path": "run.trace.json" }
    },
    "output": "build/profiles/yolov5n",
    "buffers": { "ring_events": 65536, "attribute_bytes": 1048576 }
}
```

The reader refuses an unknown key and a value out of range, naming the key. The format has a page
in `docs/reference/` and a version field.
