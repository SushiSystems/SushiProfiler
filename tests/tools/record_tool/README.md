# tests/tools/record_tool

- `test_record_tool.cpp`: one test per outcome of each command, medium except the usage ones. `validate`
  accepts a record, fails on an unsupported format version and refuses a missing file with 2; `show`
  prints only the named analysis, the default ones when none is named, and refuses an unknown analysis
  and a bad `--top`; `compare` names a faster candidate, refuses a different build type and compares it
  when the mismatch is allowed; `check` prints a passing check, fails when one check fails and refuses a
  check file that does not parse. The small tests cover usage with no command, `help`, an unknown
  command, an unknown option and an option without its value. Records and check files are written to
  the test's temporary folder; the fixture removes them in `TearDown`.
