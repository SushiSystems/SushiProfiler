# tests/session/environment

- `test_environment_contributor.cpp`: with no contributors the facts are the host's and the
  library build's; two contributors run in order, adding an environment fact and the last one's
  `build_type` replacing the library's.
- `test_record_facts.cpp`: `set_fact` adds and then replaces in place; the host facts name the
  operating system, a trimmed CPU brand and a thread count; the build facts name the compiler and
  `debug` or `release`.
