# tests/record/record_writers

- `test_terminal_writer.cpp`: the exact text of a table cut at one row and of an empty table;
  columns widen for every printed row; times in each step; ratios, counts and plain numbers.
- `test_record_file_writer.cpp` (medium, writes in the test temporary folder): the file holds
  the bytes `write_record` gives and reads back to the same record; a path in a missing folder
  is `PathUnwritable`.

`test_record_writer.cpp` checks the `RecordWriter` port's promise on `MemoryRecordWriter` and
`TerminalWriter`. `test_memory_record_writer.cpp` checks that it keeps the record and its tables and
that a failing write returns its status and keeps nothing.
