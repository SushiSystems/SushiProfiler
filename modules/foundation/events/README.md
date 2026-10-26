# events

The event every SushiProfiler record is written as, in the namespace `SushiProfiler`. The module
is header-only and depends on the standard library alone.

- `SushiProfiler/events/ids.hpp`: `SiteID` and `NameID`, 32-bit ids where 0 means none, and
  `K_MAX_SITE_ID`, 2^28 - 1.
- `SushiProfiler/events/trace_event.hpp`: `TraceEvent`, 24 trivially copyable bytes: two 64-bit
  fields, a site word and a name. `EventKind` names 14 kinds, kept in the top four bits of the
  site word by `pack_site` and read back by `kind_of` and `site_of`. Each enumerator's comment says
  what `begin`, `end` and `name` hold for it. Times are 64-bit, so no span is clipped.
  `value_bits` and `value_of` carry a counter's or value's double through the `end` field bit for
  bit, NaN payloads and signed zeros included.

| Kind | begin | end | name |
| --- | --- | --- | --- |
| Zone | start ticks | end ticks | dynamic name or 0 |
| ExternalZone | start steady ns | end steady ns | dynamic name or 0 |
| Instant | ticks | the same ticks | 0 |
| ExternalInstant | steady ns | the same steady ns | 0 |
| Counter, Value | ticks | the value's bits | 0 |
| PeriodMark | ticks | the index of the period it opens | timeline |
| PeriodBudget | ticks | the period's budget in ns | timeline |
| PeriodTag | ticks | key << 32, then value | timeline |
| FlowBegin, FlowStep, FlowEnd | ticks | the caller's item id | 0 |
| LaneBegin, LaneEnd | ticks | unused | lane |

One event per zone, written when the zone ends, replaces a begin and an end event; see
`docs/design/PROFILER/ENGINE_CHANGES.md`. The tests are in `tests/foundation/events/`.
