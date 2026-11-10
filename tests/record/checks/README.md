# tests/record/checks

- `test_check.cpp`: a time check and a plain check read with their bounds; `at_least_ns` read as the
  lower bound; a time bound on a plain subject and the reverse refused; and one test for the text of
  each other refusal (shape, subject prefix, statistic, missing statistic, missing bound, bound type,
  JSON syntax).
- `test_verdict.cpp`: over periods of 100, 120 and 110 ns, a median within bounds passes; a
  maximum above at most and a minimum below at least fail with their reasons; p90 is 118; p50
  gives the median; a warm-up period is not read; a subject without samples fails.
