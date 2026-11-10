# tests/foundation/json

- `test_json_value.cpp`: each factory makes its kind, as rows; a string answers as a string and as no
  other kind; numbers convert when they fit and are refused when they do not; an object keeps its
  order, replaces a key in place and finds a member or null; arrays append; equality compares kind
  and content, down to nested arrays and objects, as rows.
- `test_json_reader.cpp`: each document reads as its kind, as rows, and each scalar and array with its
  value; 64-bit integers at both ends and one past; `-0.0` and `1e300`; escapes and a surrogate
  pair; an error's line and column; what JSON refuses, as rows; the depth limit; a duplicate key.
- `test_json_field_reader.cpp`: one test per field kind; a strict object needs every key and refuses
  an unnamed one; an optional object accepts missing keys and refuses an unnamed one; `has`; a
  refusal names its path and the first one stays.
- `test_json_writer.cpp`: compact output; one member to a line in the indented writer and an inline
  array on one line; the escape rules and the real forms, as rows; a non-finite real as null; a value
  round trip.
