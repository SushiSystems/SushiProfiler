# tests/record/record_json

- `test_record_json.cpp`: a record that uses every table, column and kind round-trips to an
  equal record and to the same bytes; an empty record round-trips; columns sit on one line;
  non-finite reals are written as words and read back. Each refusal is checked with its full
  text: a newer version, the version before zone runs, a zone run on a site past its table, ending
  before it begins or on a thread outside the table, an unknown key, a missing key, a value out of range, a value of the
  wrong kind, columns of unequal length, a site past its table, a parent that is not earlier, a
  thread outside the table, an unknown word, and text that is not JSON.
