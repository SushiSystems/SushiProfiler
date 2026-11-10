# tests/session/registry

- `test_registry.cpp`: a source is made by name, and an unknown one is refused with the known
  names, one test each; a writer name is taken once across both kinds; `chrome_trace` streams and `terminal`
  does not, and an unknown writer lists every writer; the built-in writers read their settings
  (one test each for the terminal's `top`, its unknown key, the record writer's path given and
  missing, and the chrome trace path); contributors keep their order; a fake
  device library registers its source and contributor through `register_profiler_parts`, its
  source places the work inside each period span, counts older work as late and adds its track, and
  its contributor's fact reaches the gathered facts, one test each. The guide `docs/guides/CLIENT_LIBRARY.md` takes
  its example from this file.

The registry tests use `MemorySource` and `MemoryContributor` where a part does nothing; the fake
device library keeps its own source and contributor, since it shows how a library wires its parts
into a registry.
