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

The module depends on the standard library alone. The tests are in `tests/foundation/clock/`.
