# record_writers

Where a run record and its analysis tables go, in the namespace `SushiProfiler`. The module
depends on `analysis` and `record`, and privately on `record_json`.

- `SushiProfiler/record_writers/record_writer.hpp`: the `RecordWriter` interface,
  `write(record, tables)`, which returns a `WriteStatus`: `Written`, `PathUnwritable` or
  `StreamFailed`. Each writer keeps the part it needs and ignores the other.
- `SushiProfiler/record_writers/terminal_writer.hpp`: `TerminalWriter(out, rows)` prints each
  table under `== name ==`, with its header, at most `rows` rows (20 by default), a count of the
  rows left out, then its notes, and a blank line between tables. Columns are separated by two
  spaces. A column whose first row is text is left-aligned; every other column is right-aligned.
  Trailing spaces are trimmed. `format_cell(cell, unit)` gives each cell's text: a time in ns,
  us, ms or s, the largest unit that keeps the value at or above 1, with two decimals (three
  for s); a ratio as a percentage with one decimal; a count as a whole number; any other
  number as a whole number when it is whole, one decimal from 1000 up, else four significant
  digits.
- `SushiProfiler/record_writers/record_file_writer.hpp`: `RecordFileWriter(path)` replaces the
  file at `path` with the record's JSON form from `write_record`, written in binary so the bytes
  are the same on every platform. The tables are not saved; `sp record show` makes them again
  from the file.

The tests are in `tests/record/record_writers/`.
