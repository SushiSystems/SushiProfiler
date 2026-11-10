# tests/record/statistics

Every test uses fixed inputs and exact expected values.

- `test_sorted_samples.cpp`: sorting and the ends; the median of an odd and of an even count; type-7
  percentiles at 0, 25, 90 and 100; one sample as every percentile.
- `test_dispersion.cpp`: the mean of nothing and of values; the coefficient of variation against its
  closed form, positive for a negative mean, and 0 for one sample, for a zero mean and without a
  spread; the jitter of fixed series.
- `test_bootstrap.cpp`: the same seed gives the same interval; `Small_CharacterizationBootstrap`
  pins the default-seed interval, 11.0 to 13.5, which a Python reimplementation of the generator
  computed and which is not in the repository (RW-27 commits it and ends the characterization); the
  interval holds the median and excludes the outlier; one sample gives a point.
- `test_steady_state.cpp`: a warm-up of two batches is cut at 10; a steady series is kept whole;
  fewer than two batches give 0; a trailing partial batch is ignored.
- `test_log2_histogram.cpp`: bins at and around powers of two with empty bins kept, and samples
  at or below 0 counted apart.
