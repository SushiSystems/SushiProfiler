# tests/session/stream_writers

- `test_chrome_trace_writer.cpp`: one period streamed to a temporary file parses as JSON; a zone
  run sits on its thread's row with its time, length and group, and a lane's run on the
  `main / voice` row; the period sits on its timeline row; a counter carries its unit and value,
  an instant and a flow's begin and end carry their item; a span sits on its device track's row
  with its queue time and attributes; an unwritable path is reported and every later call fails.
- `test_stream_writer.cpp`: a writer driven through the interface receives `begin`, each period
  in order with its zone runs, and `end`.
