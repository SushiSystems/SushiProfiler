# zones

What a program writes to record, in the namespace `SushiProfiler`. The module is header-only and
depends on `events`, `sites`, `rings` and `collector`.

## Macros

`SushiProfiler/zones/macros.hpp` is the header a program includes. Each macro builds its site as
a `static constexpr Site` with a static handle, named with `__COUNTER__`, so a site costs nothing
until its first record and one acquire load after it.

The `_AT` forms take the compiled level first, so each library writes its own wrapper over its own
level option:

```cpp
#define SR_ZONE(name) SP_ZONE_AT(SR_PROFILE_LEVEL, 1, name, nullptr)
```

| Macro | Records |
| --- | --- |
| `SP_ZONE_AT(compiled, level, name, group)` | The rest of the scope as a Zone |
| `SP_ZONE_NAMED_AT(compiled, level, name, group, name_id)` | The same, named by an interned id |
| `SP_COUNTER_AT(compiled, level, name, unit, op, value)` | A Counter folded by `CounterOp::op` |
| `SP_VALUE_AT(compiled, level, name, unit, value)` | A Value kept as sampled |
| `SP_INSTANT_AT(compiled, level, name)` | An Instant |
| `SP_FLOW_AT(compiled, level, name, kind, item)` | A flow point, `kind` one of `FlowBegin`, `FlowStep`, `FlowEnd` |
| `SP_LANE_AT(compiled, lane_id)` | LaneBegin and LaneEnd around the rest of the scope |

A record of `level` above `compiled` is compiled out: a zone becomes an empty object whose
constructor does nothing, and a one-shot record sits in a discarded `if constexpr` branch. A lane
is compiled in when `compiled` is at least 1.

A program that uses SushiProfiler alone takes the direct forms over `SP_PROFILE_LEVEL`, 2 unless
it defines it before the include: `SP_ZONE(name)`, `SP_ZONE_GROUP(name, group)`,
`SP_ZONE_FINE(name)` (level 2), `SP_COUNTER(name, unit, op, value)`, `SP_VALUE(name, unit, value)`,
`SP_INSTANT(name)`, `SP_FLOW_BEGIN`, `SP_FLOW_STEP` and `SP_FLOW_END(name, item)`, and
`SP_LANE(lane_id)`. The others are level 1.

## Types and functions

- `zone.hpp`: `ScopedZone<bool Compiled>` reads the clock on entry and writes one complete Zone
  event on exit, so a zone is one 24-byte store, not a begin and an end. It writes nothing when
  it did not start or when the collector closed while it was open. `ScopedLane<bool Compiled>`
  writes LaneBegin and LaneEnd with the lane's interned name; its site is 0, so only the gate
  turns it off. `SiteSlot<bool>` is a `SiteHandle` when compiled in and the empty
  `CompiledOutHandle` when out.
- `records.hpp`: `record_counter`, `record_value`, `record_instant`, `record_flow`, and two that
  take the caller's steady-clock times, `record_instant_at` and `record_zone_at`, for spans a
  device or another clock measured. `record_zone_at` takes an optional interned name.
- `admission.hpp`: `Detail::admit`, the check every record makes: the gate, the site's enable
  byte, then the thread's ring. A closed gate costs one relaxed load. A site's first use
  registers it and may allocate; a failure there refuses the record, and no exception leaves a
  record.

The tests are in `tests/capture/zones/`.
