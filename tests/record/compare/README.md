# tests/record/compare

- `test_compare_records.cpp`: rows of one test, a candidate at half the time is `faster` with
  ratio 0.5 and one at twice the time is `slower` with ratio 2, every cell checked; overlapping
  intervals give no change; a differing build fact refuses the comparison, naming the field and both
  values, and when allowed becomes a note; periods tagged
  `layer=fused` and `layer=plain` are compared group by group; a subject in one record only has
  no row.
