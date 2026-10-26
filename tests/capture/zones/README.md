# tests/capture/zones

- `recording_fixture.hpp`: `RecordingFixture` opens the collector before each test, closes and
  empties it after, and `drain_mine()` returns what the test's thread wrote.
- `test_admission.cpp`: an enabled site gets the thread's ring; a closed collector and a site the
  filter disables get none.
- `test_zone.cpp`: a zone writes one complete event with its site's name and line; nested zones
  nest by time; a named zone carries its name; a disabled site, a zone that outlives the
  collector's close and a compiled-out zone write nothing; compiled-out zones and lanes are
  empty types.
- `test_records.cpp`: a counter keeps its operation, unit and value; a value keeps its sample;
  an external zone keeps the caller's times past 2^32; an external instant keeps its time; flows
  carry their item; a lane brackets the events inside it; compiled-out records write nothing.
- `test_macros.cpp`: every direct macro writes its kind at the default level 2.
