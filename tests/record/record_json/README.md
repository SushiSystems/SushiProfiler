# tests/record/record_json

- `test_record_json.cpp`: rows of one test, a record that uses every table, column and kind and an
  empty record each round-trip to an equal record and to the same bytes; columns sit on one line;
  non-finite reals are written as words and read back. Each refusal is checked with its full
  text, one row where several inputs break the same rule: a version other than 2 (3 and 1), a site
  past its table in a zone node or in a zone run, a thread outside the table in a zone run or in a
  period; and one test for each of a zone run ending before it begins, an unknown key, a missing
  key, a value out of range, a value of the wrong kind, columns of unequal length, a parent that is
  not earlier, an unknown word, and text that is not JSON.
