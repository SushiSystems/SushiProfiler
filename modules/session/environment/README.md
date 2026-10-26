# environment

The facts a record states about where and how it was made, in the namespace `SushiProfiler`. The
module depends on `record`.

- `SushiProfiler/environment/record_facts.hpp`: `RecordFacts` holds the record's `environment`
  and `build` facts. `set_fact` sets a fact, replacing its value in place when the name is there.
  `host_facts()` returns `os` (`Windows`, `Linux`, `macOS` or `other`), `cpu` (the brand string
  from CPUID leaves 0x80000002 to 0x80000004, or `unknown` off x86) and `hardware_threads`.
  `library_build_facts()` returns `compiler` (such as `msvc 194435217` or `clang 18.1.8`) and
  `build_type` (`debug`, or `release` when `NDEBUG` is defined) of this library's build.
- `SushiProfiler/environment/environment_contributor.hpp`: `EnvironmentContributor` is the
  interface a library implements to add its facts: SushiRuntime's adds the device, driver and
  backend, and each library adds its name, tag and commit. `gather_facts(contributors)` starts
  from the host facts and the library's build facts and runs each contributor in order, so a host
  can set its own `build_type` over the library's.

`compare_records` refuses records whose `build` facts differ, so only facts that change what is
measured go there.

The tests are in `tests/session/environment/`.
