# tests/record/record

- `test_span_table.cpp`: spans append and read back; a column added late marks earlier spans
  absent; a span added late is absent from every column; equal tables compare equal.
- `test_sample_series.cpp`: equality reaches every column.
- `test_period_record.cpp`: duration past 2^32 ns; equality reaches trees, tags and spans.
- `test_run_record.cpp`: names and sites by id; `find_name`; equality reaches the sites, the drops
  and the clock.
