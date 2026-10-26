# tests/capture/lock_wait

- `test_contended_lock.cpp`: an uncontended lock writes nothing and releases its mutex; `unlock`
  and `lock` release and take the mutex again; a lock another thread holds for 20 ms writes one
  zone at a `LockWait` site in the group `Lock wait`; a compiled-out lock waits and writes
  nothing.
- `test_named_lock_wait.cpp`: `SP_LOCK_WAIT_NAMED_AT` declares a lock held on return; an
  uncontended one writes nothing; one another thread holds writes one `LockWait` zone; the lock
  waits on a `std::condition_variable` and returns holding it; at level 0 it still locks.
