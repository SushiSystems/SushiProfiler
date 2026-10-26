# statistics

Summaries of samples, such as step times, in the namespace `SushiProfiler`. The analyses and the
comparison build on them. The module depends on nothing. Every function is deterministic:
the same input gives the same bits on every compiler and platform.

- `SushiProfiler/statistics/sorted_samples.hpp`: `SortedSamples` sorts once and answers
  `minimum`, `maximum`, `median` and `percentile(p)`. A percentile follows the type-7 rule: rank
  (n - 1) p / 100, interpolated linearly between the two samples around it. Queries other than
  `size` and `empty` need at least one sample, and no sample may be NaN.
- `SushiProfiler/statistics/dispersion.hpp`: `mean`; `coefficient_of_variation`, the sample
  standard deviation over the absolute mean, 0 for fewer than two samples or a mean of 0; and
  `jitter`, the median absolute difference between successive samples, which ignores a slow
  drift and measures period-to-period noise.
- `SushiProfiler/statistics/bootstrap.hpp`: `bootstrap_median_interval(samples, settings)`
  returns the percentile bootstrap `Interval` of the median. `BootstrapSettings` holds the
  resample count (2000), the confidence (0.95) and the seed. Draws come from SplitMix64, mapped to
  an index by multiplying the high 32 bits by the sample count and keeping the high 32 bits of the
  product. Standard-library distributions are avoided because their output differs between
  library vendors.
- `SushiProfiler/statistics/steady_state.hpp`: `mser5(series)` returns how many leading samples
  are warm-up. It averages the series in batches of `K_MSER_BATCH` (5), tries every cut d over
  the first half of the m batches, and keeps the one with the least variance of the remaining
  batch means over (m - d)^2; the earliest cut wins a tie.
- `SushiProfiler/statistics/log2_histogram.hpp`: `log2_histogram(samples)` returns a
  `Log2Histogram`: bin i counts samples in [2^(first_exponent + i), 2^(first_exponent + i + 1)),
  empty bins included, and `nonpositive` counts samples at or below 0. The exponent comes from
  `std::frexp`, so each power of two lands in its own bin exactly.

The tests are in `tests/record/statistics/`.
