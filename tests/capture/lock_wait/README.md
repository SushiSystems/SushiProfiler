# tests/capture/lock_wait

- `scripted_mutex.hpp`: `ScriptedMutex`, a Lockable that pretends another thread holds it and
  moves a `ManualTickSource` by a set wait when `lock` ends that hold, so no test needs a thread
  or a sleep to contend a lock.
- `test_contended_lock.cpp`: on a collector of its own, an uncontended lock writes nothing and
  releases its mutex at scope end; `unlock` and `lock` release and take the mutex again; a
  scripted contended lock writes one zone as long as the scripted wait, at a `LockWait` site in
  the group `Lock wait`; a compiled-out lock still locks and writes nothing. Two medium tests
  check that `SP_LOCK_WAIT_AT` reaches the process-wide collector at level 1 and 0.
- `test_named_lock_wait.cpp`: `SP_LOCK_WAIT_NAMED_AT` declares a lock held on return; an
  uncontended one writes nothing; one another thread holds writes one `LockWait` zone; the lock
  waits on a `std::condition_variable` and returns holding it; at level 0 it still locks.
