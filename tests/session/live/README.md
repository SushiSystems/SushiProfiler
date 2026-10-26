# tests/session/live

- `test_name_snapshot.cpp`: one snapshot is reused while the tables and thread names are
  unchanged and rebuilt when a name, a site, a track or a thread name changes; a snapshot keeps
  its text after the record grows; unknown ids resolve to nothing.
- `test_closed_period.cpp`: of runs lasting 10, 50, 30, 40 and 20 ns a cap of 2 keeps 50 and 40
  and counts 3 omitted; a minimum of 25 ns drops the short ones; kept runs keep their thread,
  lane and begin order; a cap of 0 keeps none; a warm-up period has none; the source period is
  untouched; the run, sequence, origin and counts are carried; every field but the runs equals the
  source's.
- `test_live_channel.cpp`: nothing before the first publication; the last N come back oldest
  first; the ring overwrites its oldest; a reader keeps a period alive after the ring wrapped and
  an unheld one is freed; `attach` and `detach` count viewers; `begin_run` empties the history and
  numbers the run; four readers against 10 000 publishes never see a sequence go backwards, a gap
  in `history(50)` or a `latest()` behind it.
- `test_live_view.cpp`: a default view reads nothing; copies and moves keep one viewer each and
  copy assignment moves the count; a view reads its channel after its owner let go.
- `test_period_publisher.cpp`: nothing is published while unwatched; sequences are consecutive
  and carry the channel's run and the origin; periods share a snapshot until a table grows; the
  record's late and dropped counts are summed.
