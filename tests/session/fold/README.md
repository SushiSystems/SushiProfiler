# tests/session/fold

- `test_lane_tracker.cpp`: nested lanes give each event the innermost open lane and close in
  turn; an end of a lane not open is unmatched; an inner lane closed first leaves the others
  open; a reset closes every lane.
- `test_period_fold.cpp`: four zones written children first nest into a parent with 70 ns of
  self time, two calls of one child merged and a second child; one site under two parents is two
  nodes; equal runs nest in ring order; trees split by thread, then lane; every run is kept with
  its depth; a zone crossing its parent's end and one ending before it begins are counted;
  counters, values, instants and flows fold into series by site, then thread, and period and lane
  events are ignored; the same events fold the same.
- `test_record_clock.cpp`: ticks map through a half-nanosecond calibration and steady times
  through the origin, both clamped at 0; steady ticks are taken as they are; a zone converts both
  fields, an external zone both from steady time and a counter only its time.
