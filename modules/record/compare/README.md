# compare

The comparison of two run records, in the namespace `SushiProfiler`. The module depends on
`analysis`, `record` and `statistics`.

- `SushiProfiler/compare/compare_records.hpp`: `compare_records(baseline, candidate, options)`
  returns a `Comparison`, a table or the reason it was refused. `CompareOptions` holds the
  `BootstrapSettings` and `allow_build_mismatch`.

Rows come from `subject_samples`, so a comparison covers every subject both records hold, under
the same names a check uses. Each row holds the tag group, the subject, its unit, the baseline
median with its bootstrap interval, the candidate's, the ratio of medians, and the ratio's
interval: the candidate's low over the baseline's high to the candidate's high over the
baseline's low. `change` is `faster` or `slower` for times, `lower` or `higher` otherwise, and
empty when the two intervals overlap. A subject in one record only has no row.

The build facts of the two records are compared first. The first fact that differs refuses the
comparison, naming the fact and both values; with `allow_build_mismatch` it becomes a note on
the table instead. When both records tag their active periods, periods are grouped by their tag
set, written as sorted `key=value` words joined by commas, and each group present in both is
compared on its own.

The tests are in `tests/record/compare/`.
