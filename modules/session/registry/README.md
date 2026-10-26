# registry

The factories a configuration names and the contributors every session runs, in the namespace
`SushiProfiler`. The module depends on `environment`, `json`, `record_writers`, `sources` and
`stream_writers`.

- `SushiProfiler/registry/registry.hpp`: `Registry` holds source factories, stream writer
  factories and record writer factories by name, and environment contributors in the order added.
  A factory returns `Made<Part>`: the part, or why its settings were refused. `make_source`,
  `make_stream_writer` and `make_record_writer` refuse an unknown name and list the known ones,
  `unknown writer "csv"; known: chrome_trace, record, terminal`; the two writer kinds share one
  namespace, and `streams(name)` says which kind a writer is. A name is added once.
  `register_builtin_writers(registry, terminal)` adds the writers this library holds:

| Writer | Kind | Settings |
| --- | --- | --- |
| `terminal` | record | `top`, the rows per table, default 20; writes to the stream it was given |
| `record` | record | `path` of the record file, joined to the output folder when relative |
| `chrome_trace` | stream | `path` of the trace file, joined to the output folder when relative |

A contributor is held by reference and must outlive every session started with the registry. A
client library therefore returns one object that owns its contributor and what its source reads;
`docs/guides/CLIENT_LIBRARY.md` states the pattern.

The tests are in `tests/session/registry/`.
