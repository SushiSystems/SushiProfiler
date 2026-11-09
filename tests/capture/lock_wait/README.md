# tests/capture/lock_wait

- `scripted_mutex.hpp`: `ScriptedMutex`, a Lockable that pretends another thread holds it and
  moves a `ManualTickSource` by a set wait when `lock` ends that hold, so no test needs a thread
  or a sleep to contend a lock.
- `test_contended_lock.cpp`: on a collector of its own, an uncontended lock writes nothing and
  releases its mutex at scope end; `unlock` and `lock` release and take the mutex again; a
  scripted contended lock writes one zone as long as the scripted wait, at a `LockWait` site in
  the group `Lock wait`; a compiled-out lock still locks and writes nothing. `SP_LOCK_WAIT_IN_AT`
  writes one `LockWait` zone at level 1 and nothing at level 0, on the same collector.
- `test_named_lock_wait.cpp`: on a collector of its own, `lock_with_wait` holds the lock on return
  and writes nothing when uncontended; a scripted contended one writes one `LockWait` zone as long
  as the scripted wait, and a compiled-out one still locks and writes nothing. `SP_LOCK_WAIT_NAMED_IN_AT`
  writes one `LockWait` zone at level 1 and nothing at level 0. One medium test checks that
  the lock waits on a `std::condition_variable` and returns holding it.
