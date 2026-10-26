# tests/foundation/events

- `test_ids.cpp`: the largest site id leaves the top four bits of a site word to the kind.
- `test_trace_event.cpp`: the event is 24 trivially copyable bytes; every kind and the lowest and
  highest site survive packing; a NaN with a payload, infinity and -0.0 keep their bits; a span of
  five seconds in nanoseconds is not clipped.
