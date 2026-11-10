# tests/session/environment

- `test_environment_contributor.cpp`: medium, with no contributors the facts are the host's and the
  library build's (it reads the machine); small, over base facts the test holds, two
  `MemoryContributor`s run in order, adding an environment fact and the last one's `build_type`
  replacing the base's.
- `test_record_facts.cpp`: small, `set_fact` appends an unknown name and replaces a known one in
  place; medium, the host facts name the operating system, a trimmed CPU brand and a thread count
  (they read the machine); small, the build facts name the compiler and `debug` or `release`.

`test_memory_contributor.cpp` checks that it states its environment and build facts and keeps the
last value of a name.
