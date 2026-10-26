# json

A JSON reader and writer, in the namespace `SushiProfiler`, so the record, the configuration and
the checks need no library outside the standard one. The module depends on nothing.

- `SushiProfiler/json/json_value.hpp`: `JsonValue`, built through `boolean`, `integer`,
  `unsigned_integer`, `real`, `string`, `array` and `object`. `JsonKind` tells signed whole
  numbers, whole numbers above `INT64_MAX` and reals apart, so a 64-bit tick count or id
  survives exactly. Objects keep their members in the order they were set and `set` replaces a
  key in its place; `find` looks a member up. `to_uint64` and `to_double` convert numbers that
  fit.
- `SushiProfiler/json/json_reader.hpp`: `read_json` reads one document by recursive descent. A
  whole number without a fraction or an exponent is `Integer` when it fits `int64_t`, else
  `Unsigned` when it fits `uint64_t`, else `Real`. A refusal returns `JsonError` with the line, the
  column in bytes and what was expected; a duplicate key, nesting deeper than
  `K_JSON_MAX_DEPTH` (128), a lone surrogate and text after the value are refused.
- `SushiProfiler/json/json_writer.hpp`: `JsonWriter` writes a document value by value into a
  string, compact or indented by two spaces. `JsonLayout::Inline` keeps a container on one line,
  as a column of numbers wants. A real is written in its shortest exact form through
  `std::to_chars`, with `.0` added to a whole real so it reads back as a real; a real that is not
  finite is written as `null`. `value` writes a whole `JsonValue`, keeping an array of scalars on
  one line.

The tests are in `tests/foundation/json/`.
