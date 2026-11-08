# tests/foundation/clock

- `test_trace_clock.cpp`: steady time never goes back; counter ticks never go back where they are
  invariant (skipped where they are not); a calibration sample's steady reading lies between two
  steady reads taken around it.
- `test_calibration.cpp`: the line maps both points exactly and the midpoint between them,
  extrapolates before the first, keeps one nanosecond per tick for equal ticks, and is the
  identity by default.
- `test_tick_source.cpp`: the promises of the `TickSource` port, run on each implementation: a
  stamp never goes back, and a sample's stamp lies between the stamps read around it;
  `ManualTickSource` small, `TraceTickSource` on the real clock medium.
- `test_manual_tick_source.cpp`: the stamp moves by exactly what `advance` adds, and a sample pairs
  the stamp with itself.
- `test_trace_tick_source.cpp`: the stamps are counter ticks exactly when the counter is
  invariant.
