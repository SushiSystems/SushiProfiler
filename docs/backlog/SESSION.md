# Session

## Remaining work

| ID | Pri | State | Title | Done when | Blocked by | Source | Opened |
| --- | --- | --- | --- | --- | --- | --- | --- |
| RW-6 | P2 | open | Apply `buffers.attribute_bytes` or remove it | a period's span attributes stop at the configured byte count and the excess is counted as dropped spans, or the key leaves the configuration format. The… |  | `docs/design/PROFILER/CAPTURE_AND_SESSION.md` | 2026-10-26 |
| RW-7 | P2 | open | Let a source state a fact of one period | `SpanSink` sets a named fact on the period it fills and the record writes it |  |  | 2026-10-26 |
| RW-8 | P2 | open | Tell a source before a period opens whether it will be recorded | a source learns the window's next recorded period ahead of its mark |  |  | 2026-10-26 |
| RW-20 | P2 | open | State and test a second start after a stop in one process | a test starts, stops and starts a session again and reads two records |  |  | 2026-10-26 |
| RW-21 | P2 | open | Add run tags to the configuration | `ProfilerConfig` holds tags and the record's `tags` table carries them |  |  | 2026-10-26 |
| RW-22 | P2 | open | Drain a burst before a ring fills | a burst after a quiet second loses no event on a ring sized by the configuration |  |  | 2026-10-26 |

## Known issues

| ID | Origin | Symptom | Workaround | Fix | Source | Opened |
| --- | --- | --- | --- | --- | --- | --- |
