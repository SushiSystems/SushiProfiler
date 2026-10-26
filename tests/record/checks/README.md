# tests/record/checks

- `test_check.cpp`: a time check and a plain check read with their bounds; statistic words; a
  time bound on a plain subject and the reverse refused; and the text of every other refusal,
  JSON errors included.
- `test_verdict.cpp`: over periods of 100, 120 and 110 ns, a median within bounds passes; a
  maximum above at most and a minimum below at least fail with their reasons; p90 is 118; a
  subject without samples fails.
