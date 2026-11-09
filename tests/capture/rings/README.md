# tests/capture/rings

- `test_thread_ring.cpp`: the capacity rounds up to a power of two of at least 64; events drain in
  order and then drain nothing more; a full ring keeps 64 of 70 and reports 6 drops once; the
  fill follows occupancy, is 0 once drained and is 1 when full; a long name is cut at 63
  characters, a second name replaces the first and an empty one reads empty; one producer and
  one consumer moving 1,000,000 events through a ring of 4,096 lose none that are not counted,
  and keep their order.
