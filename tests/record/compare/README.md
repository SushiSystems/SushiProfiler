# tests/record/compare

- `test_compare_records.cpp`: a candidate at half the time is `faster` with ratio 0.5 and every
  cell checked; overlapping intervals give no change; a slower candidate is named; a differing
  build fact refuses the comparison or, when allowed, becomes a note; periods tagged
  `layer=fused` and `layer=plain` are compared group by group; a subject in one record only has
  no row.
