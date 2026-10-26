# config

The profiler configuration, in the namespace `SushiProfiler`. The module depends on `json`. Its
JSON form is `docs/reference/CONFIG_FORMAT.md`.

- `SushiProfiler/config/profiler_config.hpp`: `ProfilerConfig` holds whether the session
  records, the runtime level, the recorded groups, the followed timeline and its
  `WindowSchedule`, the sources and writers as `NamedSettings` (a name and its settings object),
  the analyses, the output folder, the `BufferSettings`, the `RecordSettings` (`keep_periods`,
  absent keeps every period) and the `LiveSettings` of the live view. A `WindowSchedule` has
  `active_all` for the window of every period, and a `NamedSettings` holds a writer's `enabled`
  flag, which the reader takes out of the settings object the writer factory sees. Every field
  has a default.
  `read_profiler_config(text)` returns a `ConfigReadResult`: the configuration, or one refusal
  that names its JSON path. `K_CONFIG_FORMAT_VERSION` is the only version read.

The reader builds on the `json` module's `JsonFieldReader`, reading each optional key only when
it is present.

The tests are in `tests/session/config/`.
