# tests/foundation/clock

- `test_trace_clock.cpp`: steady time never goes back; counter ticks never go back where they are
  invariant (skipped where they are not); a calibration sample's steady reading lies between two
  steady reads taken around it.
- `test_calibration.cpp`: the line maps both points exactly and the midpoint between them,
  extrapolates before the first, keeps one nanosecond per tick for equal ticks, and is the
  identity by default.
