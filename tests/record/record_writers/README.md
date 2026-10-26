# tests/record/record_writers

- `test_record_writer.cpp`: a terminal writer and a test writer run through the one interface.
- `test_terminal_writer.cpp`: the exact text of a table cut at one row and of an empty table;
  columns widen for every printed row; times in each step; ratios, counts and plain numbers.
- `test_record_file_writer.cpp` (medium, writes in the test temporary folder): the file holds
  the bytes `write_record` gives and reads back to the same record; a path in a missing folder
  is `PathUnwritable`.
