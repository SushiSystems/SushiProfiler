# tests/capture/periods

- `test_period_timeline.cpp`: marks count from 0 and `current()` follows them; a mark carries its
  index and timeline; a budget from the constructor or from `mark(budget)` is written, and 0
  writes none; the index advances while the collector is closed, so the first recorded mark
  carries index 2; a tag carries its key and value; two timelines keep their own index.
