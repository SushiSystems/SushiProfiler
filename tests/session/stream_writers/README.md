# tests/session/stream_writers

- `test_chrome_trace_writer.cpp`: one period streamed to a temporary file, which the helper deletes after reading, parses as JSON; a zone
  run sits on its thread's row with its time, length and group, and a lane's run on the
  `main / voice` row; the period sits on its timeline row; a counter carries its unit and value; an
  instant and a flow's begin and end carry their item, one test each; a span sits on its device
  track's row, and its queue time and attributes are arguments; an unwritable path is reported and every later call fails.

`test_stream_writer.cpp` checks the `StreamWriter` port's promise on `MemoryStreamWriter`.
`test_memory_stream_writer.cpp` checks that it keeps periods and runs in order, that a failing write
returns its status, keeps nothing and still counts, and that it counts begins and ends.
