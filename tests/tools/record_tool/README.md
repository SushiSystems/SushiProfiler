# tests/tools/record_tool

- `test_record_tool.cpp`: `validate` accepts a record, fails on an unsupported format version
  and refuses a missing file with 2; `show` prints only the named analyses, all six by default,
  and refuses an unknown analysis and a bad `--top`; `compare` names a faster candidate, refuses
  a different build type and compares it when the mismatch is allowed; `check` prints each
  verdict, fails when one check fails and refuses a check file that does not parse; usage,
  `help`, an unknown command, an unknown option and an option without its value. Records and
  check files are written to the test's temporary folder.
