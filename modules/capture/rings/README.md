# rings

`SushiProfiler/rings/thread_ring.hpp` declares `ThreadRing`, one thread's single-producer,
single-consumer ring of `TraceEvent`s, in the namespace `SushiProfiler`. It depends on `events`.

- The capacity is rounded up to a power of two, at least 64 events.
- `push` is inline: it writes the slot and publishes the head, reading the consumer's tail only
  when its cached copy says the ring may be full. A full ring returns false and counts the drop;
  `push` never blocks, allocates, locks or calls the system, so a real-time thread may call it.
- `drain` appends every held event, oldest first, and frees their slots; `take_dropped` returns
  the drops since its previous call; `fill` returns the held share of the capacity, which the
  consumer uses to choose its next interval.
- Head, tail and the name sit on separate cache lines, so the producer and the consumer do not
  share one.
- `set_name` and `name` keep the thread's name, up to 63 characters, behind a sequence counter,
  so a name read while it is rewritten is retried rather than torn.

The layout is SushiEngine's per-thread trace buffer, moved here. The tests are in
`tests/capture/rings/`.
