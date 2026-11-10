# tests/record/record_writers

- `test_terminal_writer.cpp`: the header and first row of a table cut at one row, aligned to the
  rows it prints; the count of rows cut; the note after the rows; the exact text of a table with no
  rows; columns widen for every printed row; rows of one test give the text of a time in each step,
  a ratio, a count, a plain number and an empty cell.
- `test_record_file_writer.cpp` (medium, writes in the test temporary folder): the file holds
  the bytes `write_record` gives and reads back to the same record; a path in a missing folder
  is `PathUnwritable`.

`test_record_writer.cpp` checks the `RecordWriter` port's promise on `MemoryRecordWriter` and
`TerminalWriter`. `test_memory_record_writer.cpp` checks that it keeps the record and its tables and
that a failing write returns its status and keeps nothing.
