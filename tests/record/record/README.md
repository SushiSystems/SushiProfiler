# tests/record/record

- `test_span_table.cpp`: spans append and read back; a column added late marks earlier spans
  absent; a span added late is absent from every column; equal tables compare equal.
- `test_sample_series.cpp`: equality reaches every column.
- `test_device_span_keys.cpp`: each attribute key is its word; the ceiling facts for track
  `queue 0` are `device.queue 0.peak_flops`, `device.queue 0.peak_flops.f16` and
  `device.queue 0.peak_bytes_per_s`; an empty track
  keeps the separators.
- `test_period_record.cpp`: duration past 2^32 ns; equality reaches trees, tags, spans and every field of a zone run.
- `test_run_record.cpp`: names and sites by id; `find_name`; equality reaches the sites, the drops
  and the clock.
