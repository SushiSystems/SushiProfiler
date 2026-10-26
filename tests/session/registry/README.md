# tests/session/registry

- `test_registry.cpp`: a source is made by name and an unknown one is refused with the known
  names; a writer name is taken once across both kinds; `chrome_trace` streams and `terminal`
  does not, and an unknown writer lists every writer; the built-in writers read their settings
  and refuse an unknown key or a missing path; contributors keep their order.
