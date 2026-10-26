# tests/foundation/json

- `test_json_value.cpp`: each factory makes its kind; accessors answer only for their kind; numbers
  convert when they fit; an object keeps its order and replaces a key in place; arrays append;
  equality compares kind and content.
- `test_json_reader.cpp`: every kind; 64-bit integers at both ends and one past; `-0.0` and
  `1e300`; escapes and a surrogate pair; an error's line and column; what JSON refuses; the depth
  limit; a duplicate key.
- `test_json_field_reader.cpp`: fields of every kind; a strict object needs every key and no
  other; an optional object accepts missing keys; a refusal names its path and the first one is
  kept.
- `test_json_writer.cpp`: compact output; indented blocks with an inline array; escapes; whole,
  negative-zero, short and large reals; a non-finite real as null; a value round trip.
