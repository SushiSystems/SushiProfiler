# tests/foundation/events

- `compile_ids.cpp`: at compile time, the largest site id leaves the top four bits of a site word to
  the kind. The header has no runtime promise, so no test runs.
- `test_trace_event.cpp`: every kind with the lowest and the highest site survives packing, as rows;
  a NaN with a payload, infinity and -0.0 each keep their bits. The size and the trivial copy of the
  event are `static_assert`s in `trace_event.hpp`.
