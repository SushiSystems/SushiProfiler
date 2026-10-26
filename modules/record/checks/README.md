# checks

Checks turn a run record into pass or fail verdicts, in the namespace `SushiProfiler`. A
library's timing tests and benchmark gates read a record through them instead of timing code by
hand. The module depends on `record`, and privately on `analysis`, `json` and `statistics`.

- `SushiProfiler/checks/check.hpp`: a `Check` bounds one `Statistic` (`median`, `p50`, `p90`,
  `p95`, `p99`, `min`, `max`) of one subject with `at_most`, `at_least` or both.
  `read_checks(text)` reads a JSON array of checks and refuses, naming the JSON path, anything
  else: an unknown key, a missing statistic, no bound, an unknown statistic or subject kind,
  or a bound that is not a number. The bound keys depend on the subject's kind: `timeline:`,
  `group:`, `zone:`, `span:` and `flow:` hold times and take `at_most_ns` and `at_least_ns`;
  `counter:` and `value:` take `at_most` and `at_least`.
- `SushiProfiler/checks/verdict.hpp`: `check(record, checks)` returns one `Verdict` per check:
  whether it passed, the statistic's value and, on failure, the reason, such as
  `max is 120 ns, above at most 115 ns`. The value is taken over the record's active periods,
  from `subject_samples`. A subject the record has no samples of fails. `all_passed` says
  whether every verdict passed.

A check file:

```json
[
  {"subject": "timeline:dsp.block", "statistic": "p99", "at_most_ns": 2670000},
  {"subject": "counter:bytes", "statistic": "max", "at_most": 1048576}
]
```

Source: not compiled

The tests are in `tests/record/checks/`.
