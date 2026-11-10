# tests/session/fold

- `test_lane_tracker.cpp`: an event outside every lane is in lane 0; nested lanes give each event
  the innermost open lane; after an inner lane closes events are in the outer lane, and after the
  outer one closes in lane 0; an end of a lane not open is unmatched; an inner lane closed first
  leaves the others open; a reset closes every lane.
- `test_period_fold.cpp`: zones written children first nest into a parent whose self time is its
  inclusive time less its children's; two calls of one site under one parent merge into one node;
  one site under two parents is two nodes; equal runs nest in ring order; trees split by thread, then lane; every run is kept with
  its depth; a zone crossing its parent's end and one ending before it begins are counted;
  counters, values, instants and flows fold into series by site, then thread; period and lane
  events fold into nothing; the same events fold the same.
- `test_record_clock.cpp`: ticks map through a half-nanosecond calibration and steady times
  through the origin, each with its own test and its own clamp at 0; steady ticks are taken as they
  are; a zone converts both fields, an external zone both from steady time and a counter only its
  time, one test each.
