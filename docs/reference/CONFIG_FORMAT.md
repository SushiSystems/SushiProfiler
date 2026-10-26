# Configuration format

The profiler configuration is one JSON object, read by `read_profiler_config` in
`modules/session/config/`. A host fills a `ProfilerConfig` from its own settings, or reads this
file. Every key but the version is optional and keeps its default when left out. The reader
refuses an unknown key, a value of the wrong kind and a value out of range, naming the JSON path:
`window.active: out of range 1..4294967295`.

## Keys

| Key | Kind | Default | Holds |
| --- | --- | --- | --- |
| `sushiprofiler` | whole | required | The format version, `1`. Any other version is refused. |
| `enabled` | bool | `true` | Whether the session records at all. |
| `level` | whole, 0 to 3 | `2` | The runtime density level. It can lower what the compiled `SP_PROFILE_LEVEL` allows, never raise it. |
| `groups` | strings | all | The site groups recorded; an empty list records every group. |
| `timeline` | string | none | The timeline the window follows. |
| `window` | object | below | Which periods of that timeline are recorded. |
| `sources` | object | none | Source name to its settings object, in file order. |
| `analyses` | strings | none | The analyses run on the record when the session stops. |
| `writers` | object | none | Writer name to its settings object, in file order. |
| `output` | string | none | The folder written outputs go in. |
| `buffers` | object | below | The capture path's sizes. |
| `record` | object | below | What the run record keeps. |
| `live` | object | below | The live view of closed periods. |

`window` follows PyTorch's schedule. `skip` periods are ignored, then each cycle has `wait`
periods not recorded, `warmup` periods recorded and marked warm-up, and `active` periods
recorded. There are `repeat` cycles, and `repeat` 0 means until the session stops. `active` may
also be the string `"all"`: every period after the warm-up is recorded, with no end. Then `skip`,
`wait` and `warmup` apply once, and `repeat` must be 1 or left out.

| `window` key | Range | Default |
| --- | --- | --- |
| `skip` | 0 to 2^32 - 1 | 0 |
| `wait` | 0 to 2^32 - 1 | 0 |
| `warmup` | 0 to 2^32 - 1 | 3 |
| `active` | 1 to 2^32 - 1, or `"all"` | 20 |
| `repeat` | 0 to 2^32 - 1 | 1 |

| `buffers` key | Range | Default | Holds |
| --- | --- | --- | --- |
| `ring_events` | 64 to 2^24 | 65 536 | Events per thread ring, rounded up to a power of two |
| `attribute_bytes` | 0 to 2^32 | 1 048 576 | Reserved for a limit on the bytes of span attributes per period; read and range-checked, but nothing applies it yet (RW-6) |
| `linger_ms` | 0 to 60 000 | 100 | How long a period stays open after its end for late events |

| `record` key | Range | Default | Holds |
| --- | --- | --- | --- |
| `keep_periods` | 0 to 2^32 - 1 | all | The newest periods the stopped record keeps; 0 keeps none, and a left-out key keeps every period |

| `live` key | Range | Default | Holds |
| --- | --- | --- | --- |
| `history` | 1 to 65 536 | 300 | Closed periods `LiveView::history` can return |
| `max_runs` | 0 to 2^24 | 2048 | Zone runs kept in each published period; the longest are kept |
| `min_run_ns` | 0 to 10^9 | 1000 | Zone runs shorter than this are not published |
| `max_poll_ms` | 1 to 1000 | 4 | The longest the consumer waits between drains while a `LiveView` exists |

A source's or writer's settings object is read by that part, not by the configuration reader;
the reader only checks that each entry is an object. A writer's settings object may also hold
`"enabled": false`; the reader removes that key, and the writer starts switched off until
`Session::set_writer_enabled` turns it on.

## Example

```json
{
    "sushiprofiler": 1,
    "level": 2,
    "groups": ["Data", "Device", "Host stage"],
    "timeline": "train.step",
    "window": { "skip": 0, "wait": 0, "warmup": 3, "active": 20, "repeat": 1 },
    "sources": { "sushiruntime.device": {} },
    "analyses": ["breakdown", "device_idle", "top_spans"],
    "writers": {
        "terminal": { "top": 20 },
        "record": { "path": "run.sprec.json" }
    },
    "output": "build/profiles/yolov5n",
    "buffers": { "ring_events": 65536, "linger_ms": 100 }
}
```

Source: not compiled
