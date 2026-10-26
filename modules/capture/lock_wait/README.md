# lock_wait

The time a thread waits for a contended lock, in the namespace `SushiProfiler`. The module is
header-only and depends on `sites` and `zones`.

`SushiProfiler/lock_wait/contended_lock.hpp` declares `ContendedLock<Mutex, Compiled>`, a guard
shaped like `std::unique_lock`: it locks in its constructor and unlocks in its destructor, and
`lock`, `unlock` and `owns_lock` let `std::condition_variable_any` wait on it. Each lock first
calls `try_lock`; only when that fails does it open a zone at its site and call `lock`. An
uncontended lock therefore writes nothing and costs one `try_lock`, and the zone measures the
wait alone, not the time the lock is held.

`SP_LOCK_WAIT_AT(compiled, name, mutex)` locks `mutex` for the rest of the scope at a site of role
`LockWait` in the group `Lock wait`, level 1; `compiled` below 1 leaves a plain lock.
`SP_LOCK_WAIT(name, mutex)` takes `SP_PROFILE_LEVEL`.

The tests are in `tests/capture/lock_wait/`.
