# clock

The clock every SushiProfiler event is stamped with, in the namespace `SushiProfiler`.

- `SushiProfiler/clock/trace_clock.hpp`: `TraceClock`. `now()` reads the processor's time-stamp
  counter, one instruction with no system call: `__rdtsc` under MSVC and clang alike, from
  `<intrin.h>` or `<x86intrin.h>`. On a processor without one it reads `steady_now_ns()`.
  `ticks_are_invariant()` asks CPUID leaf 0x80000007 once whether the counter runs at one rate in
  every power state and on every core; ticks map onto steady time only when it does.
  `sample()` takes 16 brackets of two counter reads around one steady read and keeps the
  tightest, so a `CalibrationPoint` errs by at most half that bracket.
- `SushiProfiler/clock/calibration.hpp`: `CalibrationPoint` and `Calibration`, the line through
  two points that turns ticks into steady nanoseconds. It extrapolates on both sides; two points
  with equal ticks keep one nanosecond per tick, and the default is the identity.
- `SushiProfiler/clock/tick_source.hpp`: `TickSource`, the port a collector reads its stamps
  through: `now()`, `sample()` and `ticks_are_tsc()`. A stamp never goes back, and a sample's
  stamp lies between the stamps read around it.
- `SushiProfiler/clock/trace_tick_source.hpp`: `TraceTickSource`, the port on the real clock. It
  asks `ticks_are_invariant()` once and stamps with `TraceClock::now()` when it holds and with
  `steady_now_ns()` when it does not.
- `SushiProfiler/clock/manual_tick_source.hpp`: `ManualTickSource`, the in-memory port. Its stamps
  are nanoseconds that move only by `advance(elapsed_ns)`, so a test sets every time a collector
  writes.

The module depends on `version` and the standard library. The tests are in
`tests/foundation/clock/`.
