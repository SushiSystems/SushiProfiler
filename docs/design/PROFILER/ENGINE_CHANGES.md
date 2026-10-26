# What changes from SushiEngine's trace layer

| Engine | Defect | SushiProfiler |
| --- | --- | --- |
| A zone writes a begin and an end event, 48 bytes, and the capture balances them: drops unmatched ends, closes open zones | Two ring writes per zone, and a repair pass that exists only because of the split | A zone writes one complete event at its end: begin, end, site, name, 24 bytes. Nesting is rebuilt by sorting. No unmatched end can exist. A zone still open at stop is reported as open in the record, with its begin |
| Events hold `const ZoneSite*`, a pointer into the image that declared the zone | Invalid after that image unloads; cannot be written to a file | A site registers once, on first execution, into a registry that copies its strings and returns a 32-bit `SiteID`. Events hold the id |
| One `zone_gate` for every zone; level and group only at compile time | Cannot turn on one group or one level without a rebuild | The gate stays one relaxed load while off. While on, a byte per site, set from the configuration, says whether that site records |
| The drainer wakes every 5 ms | Wakes a core 200 times a second with nothing to do | The consumer polls at an interval it adapts from how full the rings were: 1 ms while a ring is over half full, doubling to 1 s while idle. No producer ever notifies, so a real-time thread makes no system call |
| `FrameSampler::mark_frame` drains and folds on the frame thread | The fold is charged to the frame it measures | `mark_period` writes one marker event. The consumer thread folds |
| Offsets inside a frame are `uint32_t` nanoseconds, held at 4.29 s | A 5 s YOLO step is clipped | 64-bit nanoseconds everywhere in the record |
| A device span that ended before the frame began is counted late and dropped | SushiRuntime reports a step's spans after the step; they would all be late | A source states the period each span belongs to. Nothing is dropped for arriving late |
| A device span holds a name and four times | No op, size, FLOPs or bytes; no link to the host zone that launched it | A span carries an attribute block and the thread and time of its submit, which places it under the host zone open at that moment |
| Tracy is a compile-time backend that replaces the recorder | A Tracy build leaves the sampler empty | Tracy, when built, is one more consumer of the same events (phase P7) |
| Modules register through static initialisers | A static library drops the translation unit and the registration with it | Each library exports one `register_profiler_parts(Registry&)` function its host calls |
| The level is one public compile definition for every target that links the module | One library cannot be compiled at a different density than another | Each library has its own level macro and option: `SP_`, `SR_`, `SB_`, `SA_PROFILE_LEVEL` |
| The TSC is read only under GCC and clang; MSVC falls back to `steady_clock` | The stack's Windows build, MSVC, pays a slower clock on every zone | `__rdtsc` and `__cpuid` from `<intrin.h>` under MSVC, the builtins elsewhere |
| Capture and analysis end at the editor window | No record, no statistics, no comparison | [CAPTURE_AND_SESSION.md](CAPTURE_AND_SESSION.md) and [RECORD_AND_ANALYSES.md](RECORD_AND_ANALYSES.md) |

Kept unchanged: the ring with its cached tail, the TSC read with its
tightest-bracket calibration, the two thread-retirement guards for the MSVC ABI, chunked storage
allocated ahead by the consumer, the fold's exact accounting, and the allocation test.
