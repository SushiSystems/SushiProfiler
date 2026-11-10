# tests/record/record

Each equality test is a row table: an equal copy of a record with every field set is equal, and a
change to one field alone makes it unequal, one row per field.

- `test_span_table.cpp`: a span appends and reads back in every field; a column added late marks
  earlier spans absent; a span added late is absent from every column; a column read from a file is
  added as it is; equal tables compare equal and an attribute value or any of the seven span columns
  breaks equality.
- `test_sample_series.cpp`: equality reaches the site, the thread and each of the four columns.
- `test_device_span_keys.cpp`: the cost keys, the label keys and the ceiling facts are written with
  the header's constants into a record, and the roofline analysis reads them back; the ceiling
  facts for track `queue 0` are `device.queue 0.peak_flops`, `device.queue 0.peak_flops.f16` and
  `device.queue 0.peak_bytes_per_s`; an empty track keeps the separators. `invocation`, `operation`,
  `launched_width` and `live_width` have no reader in this repository, so their spelling is checked
  nowhere.
- `test_period_record.cpp`: duration past 2^32 ns; equality reaches every field of a period,
  including every field of a zone run.
- `test_run_record.cpp`: a name and a site by id, an unknown id; `find_name`; equality reaches
  every table and every field of the clock and the drops.
