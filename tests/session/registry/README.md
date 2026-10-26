# tests/session/registry

- `test_registry.cpp`: a source is made by name and an unknown one is refused with the known
  names; a writer name is taken once across both kinds; `chrome_trace` streams and `terminal`
  does not, and an unknown writer lists every writer; the built-in writers read their settings
  and refuse an unknown key or a missing path; contributors keep their order; a fake
  device library registers its source and contributor through `register_profiler_parts`, its
  source collects the work inside each period span and counts older work as late, and its
  contributor's fact reaches the gathered facts. The guide `docs/guides/CLIENT_LIBRARY.md` takes
  its example from this file.
