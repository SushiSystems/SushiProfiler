# Record format

A run record is one JSON document, written by `write_record` and read by `read_record` in
`modules/record/record_json/`. This page is the format's reference: every key, its kind and what
the reader refuses. The record's meaning, such as what a period or a zone tree holds, is in
`modules/record/record/README.md`.

## Rules for every field

- Every key listed here is required and no other key is allowed. Key order does not matter to
  the reader; the writer always uses the order on this page, so one record gives one text.
- Whole numbers are written without a fraction and read exactly up to 2^64 - 1. A field whose
  type is narrower, such as a 32-bit thread index, refuses a larger value.
- Reals are written in their shortest exact form, with `.0` or an exponent so they read back as
  reals. A real that is not finite is the string `"NaN"`, `"Infinity"` or `"-Infinity"`.
- Times are nanoseconds since the session's first calibration point.
- A column is an array with one entry per row, written on one line. Columns of one table must
  have equal lengths.
- An id must point into its table: a NameID below the length of `names`, a SiteID below the
  length of `sites`, a TrackID below the length of `tracks`, and a thread index that `threads`
  lists. Entry 0 of `names`, `sites` and `tracks` is the empty entry id 0 refers to.

A refusal names the path of the field and the reason, for example
`periods[0].zones[0].site[1]: points past the sites table`. Text that is not JSON is refused with
its line and column.

## The document

| Key | Kind | Holds |
| --- | --- | --- |
| `sushiprofiler_record` | whole | The format version, `2`. Any other version is refused, naming its number; version 1, which held no zone runs, is among them. |
| `environment` | pairs | The host, the libraries and the configuration, as `[name, value]` strings. |
| `build` | pairs | Build type, optimisation and sanitisers. |
| `tags` | pairs | The run's own tags. |
| `clock` | object | `source` (`"tsc"` or `"steady"`), `nanoseconds_per_tick` and `read_cost_ns`, both reals. |
| `names` | strings | Interned names; position is the NameID. |
| `sites` | objects | One per SiteID; see below. |
| `tracks` | strings | Track names; position is the TrackID. |
| `threads` | objects | `index`, `name`, `dropped`, `late` and `unmatched`; an index appears once. |
| `drops` | object | `outside_periods`, `open_at_stop` and `spans_dropped`, whole numbers. |
| `periods` | objects | The recorded periods, in order; see below. |

A site holds `name`, `group`, `file` and `unit` as strings, `line` (32-bit) and `level` (8-bit)
as whole numbers, `role` as one of `zone`, `lock_wait`, `instant`, `counter`, `value` and `flow`,
and `op` as one of `add`, `set`, `max` and `min`.

## A period

| Key | Kind | Holds |
| --- | --- | --- |
| `index` | whole | The period's index on its timeline. |
| `timeline` | NameID | The timeline. |
| `thread` | thread | The thread that marked it. |
| `begin_ns`, `end_ns` | whole | Its start and end; the end may not come before the start. |
| `warmup` | bool | Whether the window marked it as warm-up. |
| `budget_ns` | whole | Its budget, `0` for none. |
| `tags` | pairs | `[key, value]` NameIDs. |
| `facts` | pairs | `[name, value]` strings its sources reported. |
| `zones` | objects | One zone tree per thread and lane. |
| `runs` | object | Every zone run of the period, as columns; see below. |
| `spans` | object | The span columns and their attributes. |
| `samples` | objects | One series per site and thread. |

A zone tree holds `thread`, `lane` (a NameID, `0` outside any lane) and six columns, one entry
per node: `site`, `name`, `parent`, `inclusive_ns`, `self_ns` and `calls`. `parent` is `-1` at a
root and otherwise the index of an earlier node.

`runs` holds seven columns, one entry per zone run: `thread`, `lane`, `site`, `name`, `begin_ns`,
`end_ns` and `depth`. A run may not end before it begins, its thread must be one `threads` lists,
and `depth` counts the zones of its thread and lane open around it. The writer lists runs by
thread, lane and begin. A warm-up period holds none, and `device_idle` reads them to name the
host zone open over a device gap.

`spans` holds seven columns, one entry per span: `queued_ns`, `start_ns`, `end_ns`, `track`,
`name`, `submit_thread` and `uncertainty_ns`. `attributes` lists one column per key: `key` (a
NameID, once per period), `kind` (`integer`, `real` or `name`), `values` and `present` (`0` or
`1`), each with one entry per span. `values` holds reals for a `real` column and signed whole
numbers otherwise; a `name` column's values are NameIDs. A span without the attribute has
`present` 0 and the value `0`.

A sample series holds `site`, `thread` and four columns: `time_ns`; `values`, reals, empty or one
per time; `items`, empty or one per time; and `points`, one of `begin`, `step` and `end` per item.

## Example

```json
{
  "sushiprofiler_record": 2,
  "environment": [
    ["os", "Windows"]
  ],
  "build": [],
  "tags": [],
  "clock": {"source": "tsc", "nanoseconds_per_tick": 0.2, "read_cost_ns": 6.5},
  "names": ["", "train.step"],
  "sites": [
    {"name": "", "group": "", "file": "", "line": 0, "level": 1, "role": "zone", "op": "add", "unit": ""},
    {"name": "forward", "group": "Host", "file": "train.cpp", "line": 40, "level": 1, "role": "zone", "op": "add", "unit": ""}
  ],
  "tracks": [""],
  "threads": [
    {"index": 0, "name": "main", "dropped": 0, "late": 0, "unmatched": 0}
  ],
  "drops": {"outside_periods": 0, "open_at_stop": 0, "spans_dropped": 0},
  "periods": [
    {
      "index": 0,
      "timeline": 1,
      "thread": 0,
      "begin_ns": 1000,
      "end_ns": 5001000,
      "warmup": false,
      "budget_ns": 0,
      "tags": [],
      "facts": [],
      "zones": [
        {
          "thread": 0,
          "lane": 0,
          "site": [1],
          "name": [0],
          "parent": [-1],
          "inclusive_ns": [4000000],
          "self_ns": [4000000],
          "calls": [1]
        }
      ],
      "runs": {
        "thread": [0],
        "lane": [0],
        "site": [1],
        "name": [0],
        "begin_ns": [1000],
        "end_ns": [4001000],
        "depth": [0]
      },
      "spans": {
        "queued_ns": [],
        "start_ns": [],
        "end_ns": [],
        "track": [],
        "name": [],
        "submit_thread": [],
        "uncertainty_ns": [],
        "attributes": []
      },
      "samples": []
    }
  ]
}
```

Source: not compiled
