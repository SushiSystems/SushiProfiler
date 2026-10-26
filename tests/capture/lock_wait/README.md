# tests/capture/lock_wait

- `test_contended_lock.cpp`: an uncontended lock writes nothing and releases its mutex; `unlock`
  and `lock` release and take the mutex again; a lock another thread holds for 20 ms writes one
  zone at a `LockWait` site in the group `Lock wait`; a compiled-out lock waits and writes
  nothing.
