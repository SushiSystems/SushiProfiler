# record_json

The run record's file form, in the namespace `SushiProfiler`. The format itself, key by key, is
`docs/reference/RECORD_FORMAT.md`.

- `SushiProfiler/record_json/record_json.hpp`: `write_record` returns a record as indented JSON
  with every column on one line; the same record always gives the same bytes.
  `read_record` returns a `RecordReadResult`: the record, or one refusal that names the JSON path
  and the reason. `K_RECORD_FORMAT_VERSION` is the version this build writes and the only one it
  reads, and the reader names the number of any other version in its refusal. Version 2 added the
  zone runs of each period.

The reader refuses an unknown or missing key, a value of the wrong kind or out of its field's
range, columns of unequal length, an id that points past its table or at an unlisted thread, a
parent that is not an earlier node, a period that ends before it begins, and a word the format
does not define. A record that `read_record` accepts is one the analyses can index without
checks.

Inside `source/`, `record_names.hpp` maps roles, counter operations, attribute kinds and flow
points to the format's words, the reader and writer are `record_json_reader.cpp` and
`record_json_writer.cpp`. The reader reads fields through the `json` module's `JsonFieldReader`.
The module depends on `record`, and privately on `json` and `sites`.

The tests are in `tests/record/record_json/`.
