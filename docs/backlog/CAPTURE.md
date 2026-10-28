# Capture

## Remaining work

| ID | Pri | State | Title | Done when | Blocked by | Source | Opened |
| --- | --- | --- | --- | --- | --- | --- | --- |
| RW-10 | P2 | open | Register a site before its first record | a program registers its sites at start and the first record on a real-time thread takes no lock and allocates nothing |  |  | 2026-10-26 |
| RW-11 | P2 | open | Measure a callback's load apart from its arrival interval | a period driven by a device callback reports the time the callback was busy beside the time between marks |  |  | 2026-10-26 |
| RW-12 | P2 | open | Add an aggregated zone that writes one event per period | a site sums its ticks and calls over a period and writes one event when it closes |  |  | 2026-10-26 |
| RW-13 | P2 | open | Name a counter or a value at run time | `SP_COUNTER_AT` and the value macro take a name built at run time, as `SP_ZONE_NAMED_AT` does |  |  | 2026-10-26 |
| RW-16 | P2 | open | Add a scoped period | an object marks a period at construction and closes it at destruction |  |  | 2026-10-26 |

## Known issues

| ID | Origin | Symptom | Workaround | Fix | Source | Opened |
| --- | --- | --- | --- | --- | --- | --- |
