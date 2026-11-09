# Test review: report

**Status:** Open — every test has a verdict; the plan's jobs are next

Every test in the repository has a verdict from `skills/testing/REVIEWING.md` of SushiSkills, reviewed
on 2026-11-05 as part of J15 (`docs/agent/2026_11_03_INDUSTRIAL_TESTING/` in SushiSkills). The review read
the code and ran nothing. Each record names the files it read. The tests of the shared checkers under
`tools/` are reviewed once, in SushiSkills; this report covers this repository's own tests and its own
checkers' tests.

## What the review found

- **The process-wide `Collector`.** `Small_` suites reach it through `RecordingFixture`, the real clock
  or `std::thread` (20 resizes). A collector seam comes first; the clock's own tests are medium, since
  the real clock is a boundary.
- **Test-made spies** of `StreamWriter`, `Source`, `RecordWriter` and `EnvironmentContributor`, about 30
  tests in `session/consumer` and the period assembler. Each port needs its shared in-memory adapter.
- **Sleeps used to wait** (`wait_until`, `breathe`) and assertions inside loops or conditionals: 18 in
  `tests/session/session/test_session.cpp`, which also starts a process-wide session from 5 small tests.
- **Bare tolerances** in `EXPECT_NEAR` (7), and in `analysis_record.cpp` `expect_row`, which every
  `expect_row` test calls with `1e-9` relative and `1e-12` absolute. No `tests/support/tolerance.hpp`.
- The CLI's `project` fixture and the stubs in `test_cli.py` and `test_package_contract.py` replace the
  module's own functions.
- Medium tests leave files in `::testing::TempDir()` (`test_chrome_trace_writer.cpp`,
  `test_record_tool.cpp`, `test_session.cpp`).

## Defects the review found

- `Small_Collector.OpenDiscardsEventsFromBefore` cannot fail: the fixture's `reopen()` drains the ring
  before `open()` (`collector.cpp`, lines 95 to 120).
- `test_resolve_build_type_ignores_the_type_a_tree_carries` tests nothing: `resolve_build_type` takes no
  tree (`services/project.py`, lines 112 to 115).
- `Small_Package.ReadsTheVersionThroughTheTarget` is labelled small by hand in
  `tests/package/CMakeLists.txt`; it is medium.
- `benchmark_consumer.cpp` is a `main()` that sleeps and prints; it belongs with the benchmarks.
- The bootstrap pinned-seed test rests on a Python reimplementation that is not in the repository: it is
  characterized until that oracle is committed.
- Helpers that assert without the `expect_` prefix: `refusal()`, `blame_of()`, `replaced`,
  `written_events`, `registry_with_builtins`.
- `tools/testing/check_test_additions.py` and its test are a leftover: SushiSkills removed the checker in
  phase B, and the copier never removes a file upstream dropped. Both are deleted.
- `tools/tests/test_test_budget_settings.py` skips here, since the repository has no `skills/` folder, so
  its promise is checked nowhere; it also defines `read_budget` and `differences` in the test file.

## Decisions

The owner delegated these decisions on 2026-11-05. Each ruling below is final for the plan; its reason follows it.

1. The collector is passed in. A reset owned by the fixture would keep the process-wide state that the small tests must not reach, and passing it in is the shape SOLID asks for. Every seam is added as a parameter or overload whose default keeps today's behaviour, so existing callers compile unchanged and the change is a minor version. A value passed in is preferred to a reset of process-wide state.
2. A named constant holding a hand-typed epsilon is not a typed tolerance. A tolerance names its kind (absolute, relative or ulps) and its derivation, so every such constant is redefined through `tolerance.hpp`, keeping its name. The rewrites stay rewrites, and they are mechanical.
3. T1 takes the tolerance out of the ten bare `EXPECT_NEAR` calls of `record` as well as out of `expect_row`, since its acceptance says no comparison with a tolerance spells a literal. Each call now measures against an operation count named in its file, with the count's derivation beside it. The rest of each of those records (the roofline column loop, for one) stays with M6.
4. The collector seam (T2) takes this shape. `Collector` gets a public constructor that takes a `TickSource`, a new port in `clock` with `TraceTickSource` on the real clock and `ManualTickSource` in memory, so a small test sets every stamp. Its gate, marking flag and sources flag belong to the instance. The statics stay, as the same calls on `Collector::instance()`, so the macros and SushiRuntime's `profile.hpp` compile unchanged. `ScopedZone`, `ScopedLane`, every record function, the named records, `PeriodTimeline`, `Window` and `Session::start` gain an overload that takes a `Collector&`; `Session::start(config, registry, collector)` is the call SushiRuntime RW-128 waits on. A thread caches its ring under a per-collector serial, and a collector built by its owner keeps site ids in its registry (`SiteCache::InRegistry`), because a site handle holds one id and the process-wide registry caches it there. What it costs: a closed process-wide gate is two loads instead of one, a stamp is a virtual call, and a thread that moves between collectors takes that collector's lock once per move. Not passed in yet: the lock-wait macros, and a source's `Collector::sources_wanted()`, which still reads the process-wide collector.
5. The in-memory adapters of T3 ship in the module of their port: `MemorySource` in `sources`, `MemoryStreamWriter` in `stream_writers`, `MemoryRecordWriter` in `record_writers` and `MemoryContributor` in `environment`. Each port's test file checks the port's promise on the adapter (on `TerminalWriter` too for `RecordWriter`), and a `test_memory_*.cpp` beside it checks what the adapter adds, since the mirror checker wants a test file per header. The spies of `test_source.cpp`, `test_stream_writer.cpp`, `test_record_writer.cpp`, `test_environment_contributor.cpp`, `test_stream_writer_set.cpp`, `test_registry.cpp` (the quiet parts) and three of the five in `test_period_assembler.cpp` are gone. Kept for M7: `StepSource` (appends on an unknown track) and `ProbingWriter` (reads the live channel mid-write) in `test_period_assembler.cpp`, `ProbeSource` and `LateContextSource` in `test_session.cpp` (they read `Collector::sources_wanted()`), and the fake device library of `test_registry.cpp`. With the spies, `Small_Source.ASourceStatesNoDelayByDefault` and `Small_Source.AnOverridingSourceStatesItsDelay` left; the first one's promise, that a source stating no delay holds no period open, is M7's session-level rewrite.

6. T5 fixes the three defects that sit in one test each: the collector reopen test now builds its own `Collector` on a `ManualTickSource` and leaves an event in the ring before `open()`, the no-op build-type test is deleted, and the package test is labelled medium by hand. The other five defects move to the backlog, since each is another job's work: RW-26 (`benchmark_consumer.cpp`), RW-27 (bootstrap oracle), RW-28 (`expect_` prefix), RW-29 (budget settings test). The leftover checker is already deleted (T6).

7. The orchestrator ruled on M1 on 2026-11-10, and the job followed it. First, a record that needs a missing seam gets the seam: a port or an overload with an in-memory adapter beside the production one, the default unchanged, and nothing of the kind goes into `tests/support`. `ContendedLock` and `lock_with_wait` now take a `Collector&`, and `ScriptedMutex` in `tests/capture/lock_wait/` stands in for a mutex another thread holds, so the contended tests need no thread and no sleep. Where the report said "resize to `Medium_`, seam first", the test runs `Small_` on its own collector instead; `Medium_` stays for a test that reaches the process-wide collector through a macro or starts a thread. Second, a header that loses its only runtime test keeps a `compile_<name>.cpp` of `static_assert`s listed in the module's `SOURCES` (`compile_collector_settings.cpp`, `compile_drain_batch.cpp`, `compile_site.cpp`), so the test-mirror checker still finds a file. Third, a tolerance is derived, never typed: a `Small_` test reads its numbers from its own clock (a zone of 25 ns that starts at 100 ns ends at 125 ns, compared exactly), and `EXPECT_DOUBLE_EQ` appears only where both operands are exact in binary (32 of 64 events fill 0.5). Fourth, a `Small_` collector sits on a `ManualTickSource`, and a `Small_` test asserts only values produced without a device, a file, a thread, a clock or the environment.

## Dispatches

dispatch: J15 review, 6 chunks — haiku high, 1 round each, passed, rung 1

## Totals

| Tests | Keep | Rewrite | Resize | Split | Merge | Delete | Characterize | Add |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 520 | 192 | 172 | 48 | 68 | 18 | 21 | 1 | 32 |

Kept as they stand: 192 of 520 (36%).

## Verdicts by question

| Verdict | Question | What it means | Tests |
| --- | --- | --- | --- |
| rewrite | 1 | a forbidden kind | 140 |
| split | 3 | no single promise | 68 |
| resize | 8 | wrong size | 48 |
| rewrite | 7 | a long-test sign | 22 |
| merge | 4 | a duplicate | 18 |
| delete | 1 | a forbidden kind | 12 |
| rewrite | 2 | cannot fail | 9 |
| delete | 3 | no single promise | 5 |

## Modules

| Module | Files | Tests | Keep | Rewrite | Resize | Split | Merge | Delete | Characterize |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `session` | 24 | 186 | 80 | 66 | 6 | 26 | 6 | 2 | 0 |
| `capture` | 18 | 92 | 18 | 17 | 37 | 15 | 0 | 5 | 0 |
| `record` | 34 | 124 | 76 | 24 | 0 | 9 | 10 | 4 | 1 |
| `cli/services` | 1 | 30 | 1 | 28 | 0 | 0 | 0 | 1 | 0 |
| `foundation` | 10 | 38 | 9 | 12 | 1 | 12 | 0 | 4 | 0 |
| `cli` | 6 | 34 | 8 | 23 | 1 | 0 | 2 | 0 | 0 |
| `tools` | 2 | 9 | 0 | 1 | 2 | 1 | 0 | 5 | 0 |
| `tools/record_tool` | 1 | 5 | 0 | 0 | 0 | 5 | 0 | 0 | 0 |
| `package` | 1 | 1 | 0 | 0 | 1 | 0 | 0 | 0 | 0 |
| `tools/umbrella` | 1 | 1 | 0 | 1 | 0 | 0 | 0 | 0 | 0 |

## Records

One line per test: `file::test | verdict | question | reason | files read`.

### `cli/tests/services/test_project.py`

```
test_the_default_build_type_is_debug | rewrite | 1 | forbidden kind: default value (constant K_DEFAULT_TYPE); the requested-type pass-through is asserted only here and must survive the rewrite | cli/tests/services/test_project.py; cli/sushiprofiler/services/project.py
test_a_build_without_a_type_configures_the_debug_tree | rewrite | 1 | forbidden kind: project fixture replaces find_project_root, load_config and load_build_env | cli/tests/services/test_project.py; cli/tests/conftest.py; cli/sushiprofiler/services/project.py
test_each_build_type_has_a_tree_of_its_own | rewrite | 1 | forbidden kind: project fixture; parametrised, one line | cli/tests/services/test_project.py; cli/tests/conftest.py; cli/sushiprofiler/services/project.py
test_a_configure_of_another_type_into_a_tree_is_refused | rewrite | 1 | forbidden kind: project fixture | cli/tests/services/test_project.py; cli/tests/conftest.py; cli/sushiprofiler/services/project.py
test_a_define_of_the_build_type_is_refused | rewrite | 1 | forbidden kind: project fixture | cli/tests/services/test_project.py; cli/tests/conftest.py; cli/sushiprofiler/services/project.py
test_a_stopped_gate_refuses_the_configure | rewrite | 1 | forbidden kind: project fixture | cli/tests/services/test_project.py; cli/tests/conftest.py; cli/sushiprofiler/services/project.py
test_a_configure_carries_both_launcher_entries | rewrite | 1 | forbidden kind: project fixture; also asserts a sorted argv list | cli/tests/services/test_project.py; cli/tests/conftest.py; cli/sushiprofiler/services/project.py
test_the_compile_runs_at_the_hardware_thread_count | rewrite | 1 | forbidden kind: project fixture; the hardware thread count is patched in sushicore, an environment stub that is allowed | cli/tests/services/test_project.py; cli/tests/conftest.py; cli/sushiprofiler/services/project.py
test_a_build_with_a_report_prints_it_and_writes_it_into_the_tree | rewrite | 1 | forbidden kind: project fixture | cli/tests/services/test_project.py; cli/tests/conftest.py; cli/sushiprofiler/services/project.py
test_a_build_without_a_report_prints_none | rewrite | 1 | forbidden kind: project fixture; pairs with the report test above | cli/tests/services/test_project.py; cli/tests/conftest.py; cli/sushiprofiler/services/project.py
test_build_configures_an_empty_tree_then_compiles | rewrite | 1 | forbidden kind: project fixture | cli/tests/services/test_project.py; cli/tests/conftest.py; cli/sushiprofiler/services/project.py
test_build_leaves_a_configured_tree_alone | rewrite | 1 | forbidden kind: project fixture | cli/tests/services/test_project.py; cli/tests/conftest.py; cli/sushiprofiler/services/project.py
test_build_with_a_define_reconfigures | rewrite | 1 | forbidden kind: project fixture | cli/tests/services/test_project.py; cli/tests/conftest.py; cli/sushiprofiler/services/project.py
test_build_with_a_sanitizer_uses_its_own_tree | rewrite | 1 | forbidden kind: project fixture | cli/tests/services/test_project.py; cli/tests/conftest.py; cli/sushiprofiler/services/project.py
test_build_with_benchmarks_switches_them_on_and_reconfigures | rewrite | 1 | forbidden kind: project fixture | cli/tests/services/test_project.py; cli/tests/conftest.py; cli/sushiprofiler/services/project.py
test_build_without_benchmarks_switches_them_off | rewrite | 1 | forbidden kind: project fixture | cli/tests/services/test_project.py; cli/tests/conftest.py; cli/sushiprofiler/services/project.py
test_bench_runs_every_benchmark_program_in_name_order | rewrite | 1 | forbidden kind: project fixture | cli/tests/services/test_project.py; cli/tests/conftest.py; cli/sushiprofiler/services/project.py
test_bench_without_a_benchmark_program_fails_and_says_so | rewrite | 1 | forbidden kind: project fixture | cli/tests/services/test_project.py; cli/tests/conftest.py; cli/sushiprofiler/services/project.py
test_bench_looks_in_the_release_tree_unless_told_another | rewrite | 1 | forbidden kind: project fixture | cli/tests/services/test_project.py; cli/tests/conftest.py; cli/sushiprofiler/services/project.py
test_record_runs_sp_record_of_the_asked_type_with_the_arguments | rewrite | 1 | forbidden kind: project fixture | cli/tests/services/test_project.py; cli/tests/conftest.py; cli/sushiprofiler/services/project.py
test_record_defaults_to_the_debug_tree | rewrite | 1 | forbidden kind: project fixture | cli/tests/services/test_project.py; cli/tests/conftest.py; cli/sushiprofiler/services/project.py
test_record_without_the_program_fails_and_says_so | rewrite | 1 | forbidden kind: project fixture | cli/tests/services/test_project.py; cli/tests/conftest.py; cli/sushiprofiler/services/project.py
test_test_passes_the_sizes_the_jobs_and_the_filter_to_ctest | rewrite | 1 | forbidden kind: project fixture; asserts many argv entries | cli/tests/services/test_project.py; cli/tests/conftest.py; cli/sushiprofiler/services/project.py
test_test_without_a_size_runs_small_and_medium_and_refuses_an_unfiltered_large_run | rewrite | 1 | forbidden kind: project fixture; two promises, to split after the fixture is rewritten | cli/tests/services/test_project.py; cli/tests/conftest.py; cli/sushiprofiler/services/project.py
test_test_with_package_installs_then_builds_and_runs_the_consumer | rewrite | 1 | forbidden kind: project fixture | cli/tests/services/test_project.py; cli/tests/conftest.py; cli/sushiprofiler/services/project.py
test_check_runs_every_checker_with_the_root | rewrite | 1 | forbidden kind: project fixture | cli/tests/services/test_project.py; cli/tests/conftest.py; cli/sushiprofiler/services/project.py
test_check_with_a_rule_runs_only_the_checkers_that_list_it | rewrite | 1 | forbidden kind: monkeypatches project_service.rules_of, a repository-owned function | cli/tests/services/test_project.py; cli/tests/conftest.py; cli/sushiprofiler/services/project.py
test_check_with_an_unknown_rule_fails_and_says_so | rewrite | 1 | forbidden kind: monkeypatches project_service.rules_of | cli/tests/services/test_project.py; cli/tests/conftest.py; cli/sushiprofiler/services/project.py
test_rules_of_reads_the_list_rules_flag | keep | 9 | rules_of reads the --list-rules output; the fake checker is input to the parser, and the subprocess is real | cli/tests/services/test_project.py; cli/sushiprofiler/services/project.py
test_resolve_build_type_ignores_the_type_a_tree_carries | delete | 2 | resolve_build_type takes no tree argument, so the test cannot fail for the promise; it passes with any tree state (resolve_build_type at project.py lines 112 to 115) | cli/tests/services/test_project.py; cli/sushiprofiler/services/project.py
```

### `cli/tests/test_cli.py`

```
test_help_lists_the_four_commands | merge | 4 | merge with cli/tests/test_cli.py::test_help_lists_the_docs_group; the same promise, the help lists the command surface | cli/tests/test_cli.py; cli/sushiprofiler/cli.py
test_build_passes_type_sanitizer_and_defines | rewrite | 1 | forbidden kind: replaces cli.project_service.build with a recorder in the calls fixture; asserts the full kwargs dict, a change detector | cli/tests/test_cli.py; cli/sushiprofiler/cli.py
test_build_passes_the_report_flag | rewrite | 1 | forbidden kind: calls fixture replaces the module's own service function | cli/tests/test_cli.py; cli/sushiprofiler/cli.py
test_build_passes_benchmarks | rewrite | 1 | forbidden kind: calls fixture replaces the module's own service function | cli/tests/test_cli.py; cli/sushiprofiler/cli.py
test_bench_passes_the_type | rewrite | 1 | forbidden kind: calls fixture replaces the module's own service function | cli/tests/test_cli.py; cli/sushiprofiler/cli.py
test_test_passes_labels_jobs_filter_and_package | rewrite | 1 | forbidden kind: calls fixture replaces the module's own service function | cli/tests/test_cli.py; cli/sushiprofiler/cli.py
test_test_refuses_zero_jobs | rewrite | 1 | forbidden kind: calls fixture; the refusal happens at parse time, so the fixture adds nothing | cli/tests/test_cli.py; cli/sushiprofiler/cli.py
test_check_passes_report_and_rule | rewrite | 1 | forbidden kind: calls fixture replaces the module's own service function | cli/tests/test_cli.py; cli/sushiprofiler/cli.py
test_help_lists_the_docs_group | merge | 4 | merge with cli/tests/test_cli.py::test_help_lists_the_four_commands; the same promise, the help lists the command surface | cli/tests/test_cli.py; cli/sushiprofiler/cli.py
test_docs_bundle_asks_for_a_release | keep | 9 | parse-time usage error names --release; no fixture | cli/tests/test_cli.py; cli/sushiprofiler/cli.py
test_a_failing_service_becomes_the_exit_code | rewrite | 1 | forbidden kind: monkeypatches cli.project_service.check, the module's own function | cli/tests/test_cli.py; cli/sushiprofiler/cli.py
test_record_show_passes_analyses_top_group_and_baseline | rewrite | 1 | forbidden kind: calls fixture replaces the module's own service function; asserts an exact argv list | cli/tests/test_cli.py; cli/sushiprofiler/cli.py
test_record_compare_passes_both_records_and_the_mismatch_flag | rewrite | 1 | forbidden kind: calls fixture replaces the module's own service function | cli/tests/test_cli.py; cli/sushiprofiler/cli.py
test_record_check_and_validate_pass_their_files | rewrite | 1 | forbidden kind: calls fixture replaces the module's own service function | cli/tests/test_cli.py; cli/sushiprofiler/cli.py
test_record_exits_with_the_programs_code | rewrite | 1 | forbidden kind: monkeypatches cli.project_service.record | cli/tests/test_cli.py; cli/sushiprofiler/cli.py
```

### `cli/tests/test_config.py`

```
test_a_pinned_compiler_wins | keep | 9 | a pinned compiler wins | cli/tests/test_config.py; cli/sushiprofiler/config.py
test_without_a_pin_the_clang_on_path_is_used | keep | 9 | without a pin, clang++ on PATH is used | cli/tests/test_config.py; cli/sushiprofiler/config.py
test_without_any_compiler_cmake_chooses | keep | 9 | without a compiler, cmake chooses the generator | cli/tests/test_config.py; cli/sushiprofiler/config.py
test_a_found_compiler_and_ninja_choose_ninja | keep | 9 | a found compiler and ninja choose Ninja | cli/tests/test_config.py; cli/sushiprofiler/config.py
test_a_visual_studio_generator_is_multi_config | keep | 9 | a Visual Studio generator is multi-config | cli/tests/test_config.py; cli/sushiprofiler/config.py
```

### `cli/tests/test_console.py`

```
test_importing_the_console_builds_nothing | rewrite | 1 | forbidden kind: a _private name (console._lazy.built) is read, and the module is reloaded; the lazy-import promise has no other test | cli/tests/test_console.py; cli/sushiprofiler/console.py
test_help_works_outside_a_checkout | resize | 8 | help works outside a checkout; monkeypatch.chdir changes the process environment, which small forbids; medium | cli/tests/test_console.py; cli/sushiprofiler/console.py; cli/sushiprofiler/cli.py
```

### `cli/tests/test_env.py`

```
test_without_a_compiler_the_environment_is_a_copy | rewrite | 1 | forbidden kind: monkeypatches Config.resolved_compiler, a method the repository owns | cli/tests/test_env.py; cli/sushiprofiler/env.py; cli/sushiprofiler/config.py
test_a_found_compiler_comes_first_on_path | rewrite | 1 | forbidden kind: monkeypatches Config.resolved_compiler; also writes a build folder under tmp_path, so the small label is wrong (medium) | cli/tests/test_env.py; cli/sushiprofiler/env.py; cli/sushiprofiler/config.py
```

### `cli/tests/test_main.py`

```
test_the_module_entry_prints_the_help | keep | 9 | python -m sushiprofiler runs the CLI and prints the help | cli/tests/test_main.py; cli/sushiprofiler/__main__.py
```

### `cli/tests/test_package_contract.py`

```
test_the_build_command_forwards_the_tree_and_lean | rewrite | 1 | forbidden kind: monkeypatches cli.project_service.build, the module's own function | cli/tests/test_package_contract.py; cli/sushiprofiler/cli.py; cli/sushiprofiler/services/project.py
test_a_lean_build_configures_the_named_tree_without_tests_applications_or_benchmarks | rewrite | 1 | forbidden kind: project fixture replaces find_project_root, load_config and load_build_env | cli/tests/test_package_contract.py; cli/tests/conftest.py; cli/sushiprofiler/services/project.py
test_a_default_build_asks_for_the_tests_and_the_applications | rewrite | 1 | forbidden kind: project fixture replaces the module's own functions | cli/tests/test_package_contract.py; cli/tests/conftest.py; cli/sushiprofiler/services/project.py
test_a_lean_build_reconfigures_a_tree_that_has_tests | rewrite | 1 | forbidden kind: project fixture replaces the module's own functions | cli/tests/test_package_contract.py; cli/tests/conftest.py; cli/sushiprofiler/services/project.py
test_lean_refuses_the_benchmarks | rewrite | 1 | forbidden kind: project fixture; the refusal returns before any command, so the fixture adds nothing | cli/tests/test_package_contract.py; cli/tests/conftest.py; cli/sushiprofiler/services/project.py
test_the_package_command_is_registered_with_its_options | keep | 9 | the package command is registered with its options | cli/tests/test_package_contract.py; cli/sushiprofiler/cli.py
test_the_compiler_the_environment_names_outranks_the_pinned_one | rewrite | 1 | forbidden kind: monkeypatches config._MODULE, a repository-owned object | cli/tests/test_package_contract.py; cli/sushiprofiler/config.py
test_without_the_variable_the_pinned_compiler_stays | rewrite | 1 | forbidden kind: monkeypatches config._MODULE; with test above, the two are rows once the seam exists | cli/tests/test_package_contract.py; cli/sushiprofiler/config.py
test_the_fragment_names_the_cli | rewrite | 1 | forbidden kind: constant; asserts the literal "sp", which duplicates PROFILE.program; rewrite to compare with the program the CLI registers | cli/tests/test_package_contract.py; cli/sushiprofiler/config.py
```

### `tests/capture/collector/test_collector.cpp`

```
Small_Collector.OpensOnceAndRefusesASecondOpen | split | 3 | name joins the gate opening and the refused second open; two promises | tests/capture/collector/test_collector.cpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp; modules/capture/collector/source/collector.cpp
Medium_Collector.ANewThreadOfAClosedCollectorGetsNoRing | keep | 9 | a closed collector hands no thread a ring | tests/capture/collector/test_collector.cpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp; modules/capture/collector/source/collector.cpp
Small_Collector.ADrainReturnsWhatThisThreadPushed | resize | 8 | drain returns the thread's events; reaches Collector::instance(), the process-wide instance, which small forbids; seam is an injected collector | tests/capture/collector/test_collector.cpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp; modules/capture/collector/source/collector.cpp
Medium_Collector.DropsReachTheDrain | keep | 9 | a full ring drops events and the drain reports the count | tests/capture/collector/test_collector.cpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp; modules/capture/collector/source/collector.cpp
Small_Collector.OpenDiscardsEventsFromBefore | rewrite | 2 | open discards old events; the fixture's reopen() calls close_and_empty(), which drains the ring first, so the test passes even if open() discards nothing (open() at collector.cpp lines 95 to 120); no other test holds the promise | tests/capture/collector/test_collector.cpp; modules/capture/collector/source/collector.cpp
Medium_Collector.ARegisteredThreadKeepsItsRingWhileClosed | keep | 9 | a thread registered while closed keeps a ring of the size asked | tests/capture/collector/test_collector.cpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp; modules/capture/collector/source/collector.cpp
Small_Collector.NamedThreadKeepsItsName | resize | 8 | thread name reaches the drain; reaches Collector::instance() | tests/capture/collector/test_collector.cpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp; modules/capture/collector/source/collector.cpp
Small_Collector.CalibrationPointsAdvance | resize | 8 | calibration points never move backwards; reaches Collector::instance() | tests/capture/collector/test_collector.cpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp; modules/capture/collector/source/collector.cpp
Medium_Collector.EachThreadHasItsOwnRing | rewrite | 1 | loops in the body (thread spawn, and an ASSERT and EXPECT loop) outside a row table or a named builder; only test of the per-thread ring promise | tests/capture/collector/test_collector.cpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp
Medium_Collector.AnExitedThreadIsDrainedThenFreed | keep | 9 | an exited thread is drained once with retired set, then freed | tests/capture/collector/test_collector.cpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp; modules/capture/collector/source/collector.cpp
Small_Collector.PauseClosesTheGateAndKeepsTheRings | split | 3 | name joins the gate closing and the rings kept; two results that can break apart | tests/capture/collector/test_collector.cpp; modules/capture/collector/source/collector.cpp
Small_Collector.ResumeOpensTheGateOfAnOpenSession | resize | 8 | resume reopens the gate; reaches Collector::instance() | tests/capture/collector/test_collector.cpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp; modules/capture/collector/source/collector.cpp
Small_Collector.ResumeDoesNothingWhileClosed | resize | 8 | resume does nothing while closed; reaches Collector::instance() | tests/capture/collector/test_collector.cpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp; modules/capture/collector/source/collector.cpp
Medium_SecondImage.AZoneOfTheSecondImageReachesTheCollectorOfTheExecutable | keep | 9 | a zone of the second shared image reaches the same collector | tests/capture/collector/test_collector.cpp; tests/capture/collector/second_image.cpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp
Medium_SecondImage.AThreadThatOnlyRunsTheSecondImageGetsItsRingFromTheSameCollector | keep | 9 | a thread that only runs the second image gets its ring from the same collector | tests/capture/collector/test_collector.cpp; tests/capture/collector/second_image.cpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp
Medium_SecondImage.AClosedCollectorRecordsNothingFromTheSecondImage | keep | 9 | a closed collector records nothing from any image | tests/capture/collector/test_collector.cpp; tests/capture/collector/second_image.cpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp
Small_Collector.HoldClosesTheZoneGateButKeepsMarking | split | 3 | name joins the closed zone gate and the kept marking with "But"; two results | tests/capture/collector/test_collector.cpp; modules/capture/collector/source/collector.cpp
Small_Collector.ReleaseReopensTheGateOnlyWhereTheWindowWants | split | 3 | two scenarios in one test (pause, hold, release, resume; then hold, pause, resume, release); a second action is a second test | tests/capture/collector/test_collector.cpp; modules/capture/collector/source/collector.cpp
Small_Collector.CloseClearsAHold | resize | 8 | close clears a hold; reaches Collector::instance() | tests/capture/collector/test_collector.cpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp; modules/capture/collector/source/collector.cpp
Small_Collector.HoldDoesNothingWhileClosed | resize | 8 | hold does nothing while closed; reaches Collector::instance() | tests/capture/collector/test_collector.cpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp; modules/capture/collector/source/collector.cpp
Small_Collector.SourcesWantedFollowsWantSources | delete | 1 | forbidden kind: trivial code, a getter and setter round trip of a flag; states no behaviour beyond storage | tests/capture/collector/test_collector.cpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp
```

### `tests/capture/collector/test_collector_settings.cpp`

```
Small_CollectorSettings.DefaultsKeepEverySiteInRingsOfSixtyFourKiloEvents | delete | 1 | forbidden kind: a default value; states no behaviour | tests/capture/collector/test_collector_settings.cpp; modules/capture/collector/include/SushiProfiler/collector/collector_settings.hpp
```

### `tests/capture/collector/test_drain_batch.cpp`

```
Small_DrainBatch.AnEmptyDrainHoldsNoThreadAndNoDrop | delete | 1 | forbidden kind: a default value of a plain struct; states no behaviour | tests/capture/collector/test_drain_batch.cpp; modules/capture/collector/include/SushiProfiler/collector/drain_batch.hpp
```

### `tests/capture/lock_wait/test_contended_lock.cpp`

```
Small_ContendedLock.AnUncontendedLockWritesNothing | resize | 8 | uncontended lock writes nothing and releases at scope end; drain_mine() reaches Collector::instance() | tests/capture/lock_wait/test_contended_lock.cpp; modules/capture/lock_wait/include/SushiProfiler/lock_wait/contended_lock.hpp; modules/capture/zones/include/SushiProfiler/zones/macros.hpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp
Small_ContendedLock.UnlockReleasesAndLockTakesAgain | split | 3 | name joins the unlock result and the lock result; two results that can break apart | tests/capture/lock_wait/test_contended_lock.cpp; modules/capture/lock_wait/include/SushiProfiler/lock_wait/contended_lock.hpp
Medium_ContendedLock.AContendedLockWritesOneWaitZone | rewrite | 1 | forbidden kinds: a sleep to synchronise (sleep_for 20 ms) and a spin loop in the body; only test of the ContendedLock wait zone | tests/capture/lock_wait/test_contended_lock.cpp; modules/capture/lock_wait/include/SushiProfiler/lock_wait/contended_lock.hpp
Medium_ContendedLock.ACompiledOutLockWaitsWithoutWriting | rewrite | 1 | forbidden kinds: a sleep to synchronise (sleep_for 5 ms) and a spin loop in the body; only test of the compiled-out promise | tests/capture/lock_wait/test_contended_lock.cpp; modules/capture/lock_wait/include/SushiProfiler/lock_wait/contended_lock.hpp
```

### `tests/capture/lock_wait/test_named_lock_wait.cpp`

```
Small_NamedLockWait.AnUncontendedLockWritesNothing | resize | 8 | uncontended named lock writes nothing; drain_mine() reaches Collector::instance() | tests/capture/lock_wait/test_named_lock_wait.cpp; modules/capture/lock_wait/include/SushiProfiler/lock_wait/named_lock_wait.hpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp
Small_NamedLockWait.ACompiledOutLockStillLocks | resize | 8 | compiled-out named lock still locks; drain_mine() reaches Collector::instance() | tests/capture/lock_wait/test_named_lock_wait.cpp; modules/capture/lock_wait/include/SushiProfiler/lock_wait/named_lock_wait.hpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp
Medium_NamedLockWait.AContendedLockRecordsOneLockWaitZone | rewrite | 1 | forbidden kinds: a sleep to synchronise (sleep_for 20 ms) and a spin loop in the body; only test of the named wait zone | tests/capture/lock_wait/test_named_lock_wait.cpp; modules/capture/lock_wait/include/SushiProfiler/lock_wait/named_lock_wait.hpp
Medium_NamedLockWait.TheLockPassesToAConditionVariableWait | rewrite | 1 | forbidden kind: a sleep to synchronise (sleep_for 10 ms in the setter); the condition variable waits without it | tests/capture/lock_wait/test_named_lock_wait.cpp; modules/capture/lock_wait/include/SushiProfiler/lock_wait/named_lock_wait.hpp
```

### `tests/capture/periods/test_period_observer.cpp`

```
Small_PeriodObserver.HearsEachMarkBeforeAndAfter | split | 3 | name joins the before hook and the after hook; they can break apart; the removal check is a third promise | tests/capture/periods/test_period_observer.cpp; modules/capture/periods/include/SushiProfiler/periods/period_observer.hpp; modules/capture/periods/include/SushiProfiler/periods/period_timeline.hpp; modules/capture/periods/source/period_timeline.cpp
Small_PeriodObserver.AMarkThatResumesTheGateIsWritten | resize | 8 | promise holds; PeriodTimeline's constructor interns through Collector::instance() (period_timeline.cpp line 38) | tests/capture/periods/test_period_observer.cpp; modules/capture/periods/include/SushiProfiler/periods/period_observer.hpp; modules/capture/periods/include/SushiProfiler/periods/period_timeline.hpp; modules/capture/periods/source/period_timeline.cpp
Small_PeriodObserver.AMarkThatPausesTheGateIsStillWritten | resize | 8 | promise holds; reaches Collector::instance() through PeriodTimeline | tests/capture/periods/test_period_observer.cpp; modules/capture/periods/include/SushiProfiler/periods/period_observer.hpp; modules/capture/periods/include/SushiProfiler/periods/period_timeline.hpp; modules/capture/periods/source/period_timeline.cpp
```

### `tests/capture/periods/test_period_timeline.cpp`

```
Small_PeriodTimeline.MarksCountFromZero | split | 3 | counting and the interned name are two promises; both are asserted | tests/capture/periods/test_period_timeline.cpp; modules/capture/periods/include/SushiProfiler/periods/period_timeline.hpp; modules/capture/periods/source/period_timeline.cpp
Small_PeriodTimeline.AMarkCarriesItsIndexAndTimeline | split | 3 | name joins the index and the timeline; two results that can break apart | tests/capture/periods/test_period_timeline.cpp; modules/capture/periods/include/SushiProfiler/periods/period_timeline.hpp; modules/capture/periods/source/period_timeline.cpp
Small_PeriodTimeline.AMarkCarriesItsBudget | resize | 8 | budget event written after its mark, 0 writes none; reaches Collector::instance() through the constructor | tests/capture/periods/test_period_timeline.cpp; modules/capture/periods/include/SushiProfiler/periods/period_timeline.hpp; modules/capture/periods/source/period_timeline.cpp
Small_PeriodTimeline.TheIndexAdvancesWhileTheCollectorIsClosed | resize | 8 | promise holds; reaches Collector::instance() through the constructor | tests/capture/periods/test_period_timeline.cpp; modules/capture/periods/include/SushiProfiler/periods/period_timeline.hpp; modules/capture/periods/source/period_timeline.cpp
Small_PeriodTimeline.ATagCarriesKeyAndValue | resize | 8 | key and value are one tag outcome, kept; reaches Collector::instance() through the constructor | tests/capture/periods/test_period_timeline.cpp; modules/capture/periods/include/SushiProfiler/periods/period_timeline.hpp; modules/capture/periods/source/period_timeline.cpp
Small_PeriodTimeline.TwoTimelinesKeepTheirOwnIndex | resize | 8 | two timelines count independently; reaches Collector::instance() through the constructor | tests/capture/periods/test_period_timeline.cpp; modules/capture/periods/include/SushiProfiler/periods/period_timeline.hpp; modules/capture/periods/source/period_timeline.cpp
Small_PeriodTimeline.AMarkWrittenWhileHeldReachesTheDrain | resize | 8 | mark written while held reaches the drain; reaches Collector::instance() through the constructor | tests/capture/periods/test_period_timeline.cpp; modules/capture/periods/include/SushiProfiler/periods/period_timeline.hpp; modules/capture/periods/source/period_timeline.cpp
Small_PeriodTimeline.ATagWhileHeldIsDropped | resize | 8 | tag while held is dropped; reaches Collector::instance() through the constructor | tests/capture/periods/test_period_timeline.cpp; modules/capture/periods/include/SushiProfiler/periods/period_timeline.hpp; modules/capture/periods/source/period_timeline.cpp
Medium_PeriodTimeline.AMarkFromAnUnregisteredThreadWhileHeldRegistersItsRing | keep | 9 | a thread that first marks while held registers its ring | tests/capture/periods/test_period_timeline.cpp; modules/capture/periods/include/SushiProfiler/periods/period_timeline.hpp; modules/capture/periods/source/period_timeline.cpp
Small_PeriodTimeline.ResetStartsPeriodZeroAgain | resize | 8 | reset makes the next mark period 0; reaches Collector::instance() through the constructor | tests/capture/periods/test_period_timeline.cpp; modules/capture/periods/include/SushiProfiler/periods/period_timeline.hpp; modules/capture/periods/source/period_timeline.cpp
```

### `tests/capture/rings/test_thread_ring.cpp`

```
Small_ThreadRing.CapacityRoundsUpToAPowerOfTwo | keep | 9 | capacity rounds up to a power of two of at least 64 | tests/capture/rings/test_thread_ring.cpp; modules/capture/rings/include/SushiProfiler/rings/thread_ring.hpp; modules/capture/rings/source/thread_ring.cpp
Small_ThreadRing.DrainsInOrder | rewrite | 1 | loops in the body (a push loop with ASSERT, an EXPECT loop) outside a row table; the medium test checks order only under concurrency, so this is the only single-thread order test | tests/capture/rings/test_thread_ring.cpp; modules/capture/rings/include/SushiProfiler/rings/thread_ring.hpp; modules/capture/rings/source/thread_ring.cpp
Small_ThreadRing.AFullRingDropsAndCounts | rewrite | 1 | loop in the body pushes 70 events; kept small, not deleted, because the collector-level test is the larger copy and the ring promise is seen at the smallest size | tests/capture/rings/test_thread_ring.cpp; modules/capture/rings/include/SushiProfiler/rings/thread_ring.hpp; modules/capture/rings/source/thread_ring.cpp
Small_ThreadRing.FillFollowsOccupancy | rewrite | 1 | loop in the body pushes 32 events without a named builder | tests/capture/rings/test_thread_ring.cpp; modules/capture/rings/include/SushiProfiler/rings/thread_ring.hpp; modules/capture/rings/source/thread_ring.cpp
Small_ThreadRing.NameIsTruncatedAtSixtyThree | split | 3 | the second set_name is a second action; truncation and overwrite are two promises | tests/capture/rings/test_thread_ring.cpp; modules/capture/rings/include/SushiProfiler/rings/thread_ring.hpp; modules/capture/rings/source/thread_ring.cpp
Medium_ThreadRing.OneProducerOneConsumerLoseNothingThatIsNotCounted | rewrite | 1 | loops in the body (a one-million push loop and an ASSERT_LT loop) outside a row table or a named builder; the only concurrency test of the ring | tests/capture/rings/test_thread_ring.cpp; modules/capture/rings/include/SushiProfiler/rings/thread_ring.hpp; modules/capture/rings/source/thread_ring.cpp
```

### `tests/capture/sites/test_dynamic_site_table.cpp`

```
Small_DynamicSiteTable.ReturnsOneStableHandlePerName | keep | 9 | one stable handle per name | tests/capture/sites/test_dynamic_site_table.cpp; modules/capture/sites/include/SushiProfiler/sites/dynamic_site_table.hpp; modules/capture/sites/source/dynamic_site_table.cpp
Small_DynamicSiteTable.KeepsTheFirstUnitAndOperationOfAName | split | 3 | name joins the unit and the operation; two results that can break apart | tests/capture/sites/test_dynamic_site_table.cpp; modules/capture/sites/include/SushiProfiler/sites/dynamic_site_table.hpp; modules/capture/sites/source/dynamic_site_table.cpp
Small_DynamicSiteTable.CopiesTheNameSoATemporaryMayBeFreed | keep | 9 | the name is copied; the registry reads it back after the temporary is freed | tests/capture/sites/test_dynamic_site_table.cpp; modules/capture/sites/include/SushiProfiler/sites/dynamic_site_table.hpp; modules/capture/sites/source/dynamic_site_table.cpp; modules/capture/sites/include/SushiProfiler/sites/site_registry.hpp
Small_DynamicSiteTable.RefusesNamesPastTheLimitAndCountsThem | rewrite | 1 | loop in the body inserts 4096 names with an ASSERT in the loop; a builder named for what it builds would hold it | tests/capture/sites/test_dynamic_site_table.cpp; modules/capture/sites/include/SushiProfiler/sites/dynamic_site_table.hpp; modules/capture/sites/source/dynamic_site_table.cpp
Medium_DynamicSiteTable.ManyThreadsGetTheSameHandle | rewrite | 1 | loops in the body (thread spawn and a per-thread call loop) outside a row table or named helper | tests/capture/sites/test_dynamic_site_table.cpp; modules/capture/sites/include/SushiProfiler/sites/dynamic_site_table.hpp; modules/capture/sites/source/dynamic_site_table.cpp
```

### `tests/capture/sites/test_name_table.cpp`

```
Small_NameTable.SameTextSameId | keep | 9 | one id per text, different text a different id | tests/capture/sites/test_name_table.cpp; modules/capture/sites/include/SushiProfiler/sites/name_table.hpp; modules/capture/sites/source/name_table.cpp
Small_NameTable.EmptyTextIsZero | keep | 9 | empty text is id 0 and is not stored | tests/capture/sites/test_name_table.cpp; modules/capture/sites/include/SushiProfiler/sites/name_table.hpp; modules/capture/sites/source/name_table.cpp
Small_NameTable.AnUnknownIdReadsEmpty | keep | 9 | an id never given reads empty | tests/capture/sites/test_name_table.cpp; modules/capture/sites/include/SushiProfiler/sites/name_table.hpp; modules/capture/sites/source/name_table.cpp
```

### `tests/capture/sites/test_site.cpp`

```
Small_Site.AHandleStartsUnregistered | delete | 1 | forbidden kind: a default value (handle id 0 at construction); the promise is trivial state, not behaviour; the registry's use of id 0 is covered by id_of tests | tests/capture/sites/test_site.cpp; modules/capture/sites/include/SushiProfiler/sites/site.hpp
```

### `tests/capture/sites/test_site_registry.cpp`

```
Small_SiteRegistry.OneSiteGetsOneIdForEveryHandleCall | keep | 9 | one id per site however often it is asked | tests/capture/sites/test_site_registry.cpp; modules/capture/sites/include/SushiProfiler/sites/site_registry.hpp; modules/capture/sites/source/site_registry.cpp
Small_SiteRegistry.TwoSitesWithEqualTextAreTwoSites | keep | 9 | two sites with equal text are two sites | tests/capture/sites/test_site_registry.cpp; modules/capture/sites/include/SushiProfiler/sites/site_registry.hpp; modules/capture/sites/source/site_registry.cpp
Small_SiteRegistry.TheFilterDisablesALevelAboveIt | keep | 9 | the level filter disables a level above it | tests/capture/sites/test_site_registry.cpp; modules/capture/sites/include/SushiProfiler/sites/site_registry.hpp; modules/capture/sites/source/site_registry.cpp
Small_SiteRegistry.TheFilterKeepsOnlyNamedGroupsAndUngroupedSites | split | 3 | name joins the named-group rule and the ungrouped-site rule; two promises | tests/capture/sites/test_site_registry.cpp; modules/capture/sites/include/SushiProfiler/sites/site_registry.hpp; modules/capture/sites/source/site_registry.cpp
Small_SiteRegistry.AFilterAppliesToSitesRegisteredLater | split | 3 | the second apply is a second action; applying a changed filter to registered sites is its own promise | tests/capture/sites/test_site_registry.cpp; modules/capture/sites/include/SushiProfiler/sites/site_registry.hpp; modules/capture/sites/source/site_registry.cpp
Small_SiteRegistry.ASiteBeyondCapacityIsCountedNotRecorded | keep | 9 | a site beyond capacity gets id 0 and is counted once; lines 142 and 143 repeat the same EXPECT_EQ on the third id; a copy error to remove when the file is next edited | tests/capture/sites/test_site_registry.cpp; modules/capture/sites/include/SushiProfiler/sites/site_registry.hpp; modules/capture/sites/source/site_registry.cpp
Small_SiteRegistry.InfoCopiesTheStrings | split | 3 | copying the strings and mapping the eight fields are two promises | tests/capture/sites/test_site_registry.cpp; modules/capture/sites/include/SushiProfiler/sites/site_registry.hpp; modules/capture/sites/source/site_registry.cpp
Medium_SiteRegistry.ConcurrentFirstUseGivesOneId | rewrite | 1 | loops in the body (thread spawn and an EXPECT loop over ids) outside a row table or named helper | tests/capture/sites/test_site_registry.cpp; modules/capture/sites/include/SushiProfiler/sites/site_registry.hpp; modules/capture/sites/source/site_registry.cpp
```

### `tests/capture/zones/test_admission.cpp`

```
Small_Admission.AnEnabledSiteGetsTheThreadsRing | rewrite | 6 | admission: an enabled site gets the calling thread's ring and a nonzero site id; the expected value is Collector::ring(), which admit returns too, so two nulls pass; assert the ring is non-null and equal to Collector::ring() | tests/capture/zones/test_admission.cpp; tests/capture/zones/recording_fixture.hpp; modules/capture/zones/include/SushiProfiler/zones/admission.hpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp
Small_Admission.AClosedCollectorAdmitsNothing | resize | 8 | admission: a closed collector admits no record; Small_ forbids a process-wide instance and the test reaches Collector::instance() through the fixture and admit; resize to Medium_ now, and the collector passed in at the shell is the seam | tests/capture/zones/test_admission.cpp; tests/capture/zones/recording_fixture.hpp; modules/capture/zones/include/SushiProfiler/zones/admission.hpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp
Small_Admission.ADisabledSiteIsRefused | resize | 8 | admission: a site the filter disables is refused; same process-wide instance as above; resize to Medium_, seam first for Small_ | tests/capture/zones/test_admission.cpp; tests/capture/zones/recording_fixture.hpp; modules/capture/zones/include/SushiProfiler/zones/admission.hpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp
```

### `tests/capture/zones/test_macros.cpp`

```
Small_Macros.EachDirectMacroWritesItsKind | rewrite | 7 | macros: each direct macro writes its event kind, in order, at the default level; the body runs eleven macros and makes one check, a sign of several promises; rewrite as one row per macro and kind | tests/capture/zones/test_macros.cpp; tests/capture/zones/recording_fixture.hpp; modules/capture/zones/include/SushiProfiler/zones/macros.hpp
Small_Macros.TheRecordingThreadNeverAllocates | rewrite | 7 | macros: the recording thread allocates nothing after each site's first use; the body holds a 100,000-round loop, which the rule forbids outside a row table or a named builder; move the loop into a builder named for what it runs, keep one measured count. The replacement of the global operator new is test instrumentation, not a repository-owned type, so it is not a forbidden mock. Size stays small | tests/capture/zones/test_macros.cpp; tests/capture/zones/allocation_counter.hpp; tests/capture/zones/allocation_counter.cpp; tests/capture/zones/recording_fixture.hpp
```

### `tests/capture/zones/test_named_macros.cpp`

```
Small_NamedMacros.ThreadNameIsCompiledOutAtLevelZero | resize | 8 | named_macros: at level 0 SP_THREAD_NAME names nothing and does not evaluate its argument; the test starts a std::thread and reaches Collector::instance(), which Small_ forbids; resize to Medium_. The compiled-out part needs neither a thread nor the instance | tests/capture/zones/test_named_macros.cpp; tests/capture/zones/recording_fixture.hpp; modules/capture/zones/include/SushiProfiler/zones/named_macros.hpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp
Small_NamedMacros.NamedRecordsAreCompiledOutAtLevelZero | resize | 8 | named_macros: at level 0 named counters and values record nothing and do not evaluate their name; the first assertion needs no instance, the drained check reaches Collector::instance(); resize to Medium_, seam first for the drained check | tests/capture/zones/test_named_macros.cpp; tests/capture/zones/recording_fixture.hpp; modules/capture/zones/include/SushiProfiler/zones/named_macros.hpp; modules/capture/zones/include/SushiProfiler/zones/named_records.hpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp
```

### `tests/capture/zones/test_named_records.cpp`

```
Small_NamedRecords.ACounterNamedAtRunTimeFoldsPerName | split | 3 | named_records: a counter named at run time gets one site per name, carrying that name; named_records: a counter named at run time has the operation and unit it was given; the name says folds, but the test checks sites, not a fold; split into the two rows | tests/capture/zones/test_named_records.cpp; tests/capture/zones/recording_fixture.hpp; modules/capture/zones/include/SushiProfiler/zones/named_records.hpp; modules/capture/sites/include/SushiProfiler/sites/site_registry.hpp
Small_NamedRecords.AValueNamedAtRunTimeKeepsEverySample | resize | 8 | named_records: a value named at run time keeps every sample under one site per name; reaches Collector::instance(), which Small_ forbids; resize to Medium_. Note: the kind of events[0] is not checked | tests/capture/zones/test_named_records.cpp; tests/capture/zones/recording_fixture.hpp; modules/capture/zones/include/SushiProfiler/zones/named_records.hpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp
Small_NamedRecords.ANameIsNotBuiltWhileTheGateIsClosed | resize | 8 | named_records: the name is not built while the gate is closed; reaches Collector::instance() through hold(), which Small_ forbids; resize to Medium_, seam first | tests/capture/zones/test_named_records.cpp; tests/capture/zones/recording_fixture.hpp; modules/capture/zones/include/SushiProfiler/zones/named_macros.hpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp
Small_NamedRecords.NamesAreKeptPerRole | resize | 8 | named_records: a counter and a value of one name are two sites; reaches Collector::instance(); resize to Medium_, seam first | tests/capture/zones/test_named_records.cpp; tests/capture/zones/recording_fixture.hpp; modules/capture/zones/include/SushiProfiler/zones/named_records.hpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp
Small_NamedRecords.ThreadNameNamesTheRing | resize | 8 | named_records: SP_THREAD_NAME names the calling thread's ring; std::thread and Collector::instance(), both forbidden in Small_; resize to Medium_, where threads are allowed | tests/capture/zones/test_named_records.cpp; tests/capture/zones/recording_fixture.hpp; modules/capture/zones/include/SushiProfiler/zones/named_macros.hpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp
```

### `tests/capture/zones/test_records.cpp`

```
Small_Records.ACounterKeepsItsOperationAndValue | split | 3 | records: a counter keeps its value; records: a counter site keeps its operation and unit; the name joins two results that can break apart | tests/capture/zones/test_records.cpp; tests/capture/zones/recording_fixture.hpp; modules/capture/zones/include/SushiProfiler/zones/records.hpp
Small_Records.AValueKeepsItsSample | resize | 8 | records: a value keeps its sample; reaches Collector::instance(); resize to Medium_, seam first | tests/capture/zones/test_records.cpp; tests/capture/zones/recording_fixture.hpp; modules/capture/zones/include/SushiProfiler/zones/records.hpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp
Small_Records.AnExternalZoneKeepsTheCallersTimes | resize | 8 | records: an external zone keeps the caller's begin and end, past 32 bits; reaches Collector::instance() through the fixture; resize to Medium_, seam first | tests/capture/zones/test_records.cpp; tests/capture/zones/recording_fixture.hpp; modules/capture/zones/include/SushiProfiler/zones/records.hpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp
Small_Records.AnExternalInstantKeepsTheCallersTime | resize | 8 | records: an external instant keeps the caller's time; reaches Collector::instance(); resize to Medium_, seam first | tests/capture/zones/test_records.cpp; tests/capture/zones/recording_fixture.hpp; modules/capture/zones/include/SushiProfiler/zones/records.hpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp
Small_Records.FlowsCarryTheirId | rewrite | 7 | records: a flow point carries its item id; the body holds a for loop over the drained events, which the straight-line rule forbids; compare each event in a row table or assert the three events by index | tests/capture/zones/test_records.cpp; tests/capture/zones/recording_fixture.hpp; modules/capture/zones/include/SushiProfiler/zones/macros.hpp
Small_Records.ALaneBracketsItsEvents | resize | 8 | records: a lane brackets the events recorded inside it; reaches Collector::instance(); resize to Medium_, seam first | tests/capture/zones/test_records.cpp; tests/capture/zones/recording_fixture.hpp; modules/capture/zones/include/SushiProfiler/zones/zone.hpp; modules/capture/zones/include/SushiProfiler/zones/macros.hpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp
Small_Records.ACompiledOutRecordWritesNothing | resize | 8 | records: compiled-out records write nothing; the check drains the collector, which is Collector::instance(); resize to Medium_, seam first | tests/capture/zones/test_records.cpp; tests/capture/zones/recording_fixture.hpp; modules/capture/zones/include/SushiProfiler/zones/macros.hpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp
```

### `tests/capture/zones/test_zone.cpp`

```
Small_Zone.WritesOneCompleteEvent | resize | 8 | zone: a scoped zone writes one complete event with its site's name, line and level; reaches Collector::instance(); resize to Medium_, seam first; the line check uses __LINE__ + 2 and matches the SP_ZONE_AT line | tests/capture/zones/test_zone.cpp; tests/capture/zones/recording_fixture.hpp; modules/capture/zones/include/SushiProfiler/zones/zone.hpp; modules/capture/zones/include/SushiProfiler/zones/macros.hpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp; modules/capture/sites/include/SushiProfiler/sites/site_registry.hpp
Small_Zone.NestedZonesNestByTime | resize | 8 | zone: nested zones nest by time; reaches Collector::instance(); resize to Medium_, seam first | tests/capture/zones/test_zone.cpp; tests/capture/zones/recording_fixture.hpp; modules/capture/zones/include/SushiProfiler/zones/zone.hpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp
Small_Zone.ANamedZoneCarriesItsName | resize | 8 | zone: a named zone carries its dynamic name; reaches Collector::instance(); resize to Medium_, seam first | tests/capture/zones/test_zone.cpp; tests/capture/zones/recording_fixture.hpp; modules/capture/zones/include/SushiProfiler/zones/zone.hpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp
Small_Zone.ADisabledSiteWritesNothing | resize | 8 | zone: a zone at a site the filter disables writes nothing; reaches Collector::instance(); resize to Medium_, seam first; SiteFilter groups name the groups that record, so the Device zone is disabled as the test expects | tests/capture/zones/test_zone.cpp; tests/capture/zones/recording_fixture.hpp; modules/capture/zones/include/SushiProfiler/zones/zone.hpp; modules/capture/sites/include/SushiProfiler/sites/site_registry.hpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp
Small_Zone.AZoneClosedAfterTheCollectorClosesWritesNothing | resize | 8 | zone: a zone whose collector closes before it ends writes nothing; reaches Collector::instance(); resize to Medium_, seam first | tests/capture/zones/test_zone.cpp; tests/capture/zones/recording_fixture.hpp; modules/capture/zones/include/SushiProfiler/zones/zone.hpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp
Small_Zone.ACompiledOutZoneIsEmpty | delete | 4 | zone: a compiled-out zone is an empty type; the static_assert at test_zone.cpp line 55 proves ScopedZone<false> is empty, and SP_ZONE_AT(1, 3) selects that type by its constant condition, so the runtime drain repeats the proof | tests/capture/zones/test_zone.cpp; modules/capture/zones/include/SushiProfiler/zones/zone.hpp; modules/capture/zones/include/SushiProfiler/zones/macros.hpp
```

### `tests/foundation/clock/test_calibration.cpp`

```
Small_Calibration.MapsBothEndsExactly | split | 3 | calibration: the calibration maps both points to their nanoseconds exactly; calibration: the calibration maps ticks between the points along their line; calibration: nanoseconds_per_tick reports the slope of the line; the checks are different results of different calls | tests/foundation/clock/test_calibration.cpp; modules/foundation/clock/include/SushiProfiler/clock/calibration.hpp
Small_Calibration.ExtrapolatesBeforeTheBegin | keep | 9 | calibration: the calibration extrapolates before the first point along the same line; 500 ticks give 4750 ns by the line through the two points; the header documents extrapolation | tests/foundation/clock/test_calibration.cpp; modules/foundation/clock/include/SushiProfiler/clock/calibration.hpp
Small_Calibration.EqualTicksKeepOneNanosecondPerTick | keep | 9 | calibration: two points with equal ticks keep one nanosecond per tick from the first point; the header documents the rule; 110 ticks give 910 ns from the point at 900 | tests/foundation/clock/test_calibration.cpp; modules/foundation/clock/include/SushiProfiler/clock/calibration.hpp
Small_Calibration.DefaultIsTheIdentity | rewrite | 1 | calibration: a default calibration is the identity; the check is a default value, a forbidden kind; the identity is documented, so the promise is real and no other test holds it; rewrite the check against the documented identity | tests/foundation/clock/test_calibration.cpp; modules/foundation/clock/include/SushiProfiler/clock/calibration.hpp
```

### `tests/foundation/clock/test_trace_clock.cpp`

```
Small_TraceClock.SteadyTimeNeverGoesBack | rewrite | 7 | trace_clock: steady time never goes back between reads; the body holds a 1000-read loop with an assertion, which the straight-line rule forbids; move the loop into a builder. Size is medium: the test reads the real steady clock, which Small_ forbids | tests/foundation/clock/test_trace_clock.cpp; modules/foundation/clock/include/SushiProfiler/clock/trace_clock.hpp
Small_TraceClock.TicksNeverGoBackWhenInvariant | rewrite | 7 | trace_clock: ticks never go back where they are invariant; the body holds a loop and an if with a skip, which the straight-line rule forbids; move the loop into a builder and the skip into the setup. Size is medium: real TSC reads | tests/foundation/clock/test_trace_clock.cpp; modules/foundation/clock/include/SushiProfiler/clock/trace_clock.hpp
Small_TraceClock.SampleBracketsTheSteadyReading | resize | 8 | trace_clock: sample brackets the steady reading; the test reads the real steady clock through sample(), which Small_ forbids and which has no injected clock; resize to Medium_, and an injected clock is the seam | tests/foundation/clock/test_trace_clock.cpp; modules/foundation/clock/include/SushiProfiler/clock/trace_clock.hpp; modules/foundation/clock/include/SushiProfiler/clock/calibration.hpp
```

### `tests/foundation/events/test_ids.cpp`

```
Small_IDs.TheLargestSiteLeavesFourBitsForTheKind | delete | 1 | the test checks the constant K_MAX_SITE_ID against a literal, a forbidden kind; the promise that kind and site do not overlap is tested by tests/foundation/events/test_trace_event.cpp::Small_TraceEvent.KindAndSiteRoundTrip at the same largest site | tests/foundation/events/test_ids.cpp; tests/foundation/events/test_trace_event.cpp; modules/foundation/events/include/SushiProfiler/events/ids.hpp
```

### `tests/foundation/events/test_trace_event.cpp`

```
Small_TraceEvent.IsTwentyFourTriviallyCopyableBytes | delete | 1 | the test runs at run time the static_assert pair that trace_event.hpp already holds at compile time, a forbidden kind; the header's static_asserts test the same promise | tests/foundation/events/test_trace_event.cpp; modules/foundation/events/include/SushiProfiler/events/trace_event.hpp
Small_TraceEvent.KindAndSiteRoundTrip | rewrite | 7 | events: every kind and the lowest and highest site survive packing; the body holds nested loops over kinds and sites, which the straight-line rule forbids; rewrite as rows of kind and site | tests/foundation/events/test_trace_event.cpp; modules/foundation/events/include/SushiProfiler/events/trace_event.hpp; modules/foundation/events/include/SushiProfiler/events/ids.hpp
Small_TraceEvent.ANaNValueKeepsItsBits | split | 3 | trace_event: a NaN with a payload, infinity and -0.0 keep their bits; the three values can break apart, so one test each: the NaN payload, infinity, and the sign of zero | tests/foundation/events/test_trace_event.cpp; modules/foundation/events/include/SushiProfiler/events/trace_event.hpp
Small_TraceEvent.ASpanLongerThanThirtyTwoBitsSurvives | delete | 4 | trace_event: a span longer than 32 bits survives in the end field; the test assigns fields and subtracts them and never reaches the code; the same promise runs through the real path in tests/capture/zones/test_records.cpp::Small_Records.AnExternalZoneKeepsTheCallersTimes | tests/foundation/events/test_trace_event.cpp; tests/capture/zones/test_records.cpp; modules/foundation/events/include/SushiProfiler/events/trace_event.hpp
```

### `tests/foundation/json/test_json_field_reader.cpp`

```
Small_JsonFieldReader.ReadsFieldsOfEveryKind | rewrite | 7 | json_field_reader: each field reads into its out parameter by kind; five fields through five accessors with different results, one row table | tests/foundation/json/test_json_field_reader.cpp; modules/foundation/json/include/SushiProfiler/json/json_field_reader.hpp; modules/foundation/json/source/json_field_reader.cpp
Small_JsonFieldReader.AStrictObjectNeedsEveryKeyAndNoOther | split | 3 | json_field_reader: a strict object needs every key; json_field_reader: a strict object refuses any key it does not name; the two refusals can break apart | tests/foundation/json/test_json_field_reader.cpp; modules/foundation/json/include/SushiProfiler/json/json_field_reader.hpp
Small_JsonFieldReader.AnOptionalObjectAcceptsMissingKeys | split | 3 | json_field_reader: an optional object accepts missing keys; json_field_reader: has reports whether a key is present; json_field_reader: an optional object refuses a key it does not name; three results that can break apart | tests/foundation/json/test_json_field_reader.cpp; modules/foundation/json/include/SushiProfiler/json/json_field_reader.hpp
Small_JsonFieldReader.ARefusalNamesItsPathAndKeepsTheFirst | split | 3 | json_field_reader: a refusal names its path; json_field_reader: the first refusal stays; the two can break apart | tests/foundation/json/test_json_field_reader.cpp; modules/foundation/json/include/SushiProfiler/json/json_field_reader.hpp
```

### `tests/foundation/json/test_json_reader.cpp`

```
Small_JsonReader.ReadsEveryKind | rewrite | 7 | json_reader: every JSON kind reads into the kind it names with its value; seven fields read through different accessors, one row table of key, kind and value | tests/foundation/json/test_json_reader.cpp; modules/foundation/json/include/SushiProfiler/json/json_reader.hpp; modules/foundation/json/include/SushiProfiler/json/json_value.hpp
Small_JsonReader.KeepsSixtyFourBitIntegersExact | split | 3 | json_reader: 64-bit integers read exactly and a number past 64 bits reads as a real; the name and the three documents are two promises: the exact range and the overflow to Real | tests/foundation/json/test_json_reader.cpp; modules/foundation/json/include/SushiProfiler/json/json_reader.hpp; modules/foundation/json/include/SushiProfiler/json/json_value.hpp
Small_JsonReader.ReadsRealsWithExponentsAndSignedZero | split | 3 | json_reader: a real keeps the sign of zero; json_reader: a real keeps its exponent; the two can break apart | tests/foundation/json/test_json_reader.cpp; modules/foundation/json/include/SushiProfiler/json/json_reader.hpp
Small_JsonReader.DecodesEscapesAndSurrogatePairs | split | 3 | json_reader: escapes decode to UTF-8; json_reader: surrogate pairs decode to one UTF-8 character; the name joins two promises that can break apart | tests/foundation/json/test_json_reader.cpp; modules/foundation/json/include/SushiProfiler/json/json_reader.hpp
Small_JsonReader.AnErrorNamesItsLineAndColumn | keep | 9 | json_reader: an error names the line and column of the first refusal; the expected column 8 is the start of the failed literal: json_reader.cpp matches "true" at the position of the 't' and fails there with the position unchanged | tests/foundation/json/test_json_reader.cpp; modules/foundation/json/include/SushiProfiler/json/json_reader.hpp; modules/foundation/json/source/json_reader.cpp
Small_JsonReader.RefusesWhatJsonDoesNotAllow | rewrite | 7 | json_reader: text JSON does not allow is refused; nine inputs of one promise written as copies, which the rows rule forbids; one parametrised test over the inputs | tests/foundation/json/test_json_reader.cpp; modules/foundation/json/include/SushiProfiler/json/json_reader.hpp
Small_JsonReader.RefusesNestingDeeperThanTheLimit | keep | 9 | json_reader: nesting past K_JSON_MAX_DEPTH is refused; the boundary pair 128 and 129 is checked; json_reader.cpp line 186 refuses at the limit | tests/foundation/json/test_json_reader.cpp; modules/foundation/json/include/SushiProfiler/json/json_reader.hpp; modules/foundation/json/source/json_reader.cpp
Small_JsonReader.ADuplicateKeyIsRefused | keep | 9 | json_reader: a duplicate key is refused and named; json_reader.cpp lines 241 and 242 refuse the second key and name it in the message | tests/foundation/json/test_json_reader.cpp; modules/foundation/json/source/json_reader.cpp
```

### `tests/foundation/json/test_json_value.cpp`

```
Small_JsonValue.EachFactoryMakesItsKind | rewrite | 7 | json_value: each factory makes the kind it names; eight calls with eight expected kinds, one row table | tests/foundation/json/test_json_value.cpp; modules/foundation/json/include/SushiProfiler/json/json_value.hpp
Small_JsonValue.AccessorsAnswerOnlyForTheirKind | rewrite | 7 | json_value: accessors answer only for their own kind; four calls with different results, one row table of accessor and kind | tests/foundation/json/test_json_value.cpp; modules/foundation/json/include/SushiProfiler/json/json_value.hpp
Small_JsonValue.NumbersConvertWhenTheyFit | rewrite | 7 | json_value: to_uint64 and to_double convert exactly the numbers that fit; six calls with different results, one row table | tests/foundation/json/test_json_value.cpp; modules/foundation/json/include/SushiProfiler/json/json_value.hpp
Small_JsonValue.AnObjectKeepsItsMembersInOrderAndFindsThem | split | 3 | json_value: an object keeps its members in the order set, and set replaces in place; json_value: find returns the member named, or null; the name joins two results that can break apart | tests/foundation/json/test_json_value.cpp; modules/foundation/json/include/SushiProfiler/json/json_value.hpp
Small_JsonValue.AnArrayAppends | keep | 9 | json_value: push_back appends to an array; two appends, then the element at index 1 is the second value | tests/foundation/json/test_json_value.cpp; modules/foundation/json/include/SushiProfiler/json/json_value.hpp
Small_JsonValue.EqualityComparesKindAndContent | rewrite | 7 | json_value: equality compares kind and content; four pairs with different expected truth, one row table of pairs | tests/foundation/json/test_json_value.cpp; modules/foundation/json/include/SushiProfiler/json/json_value.hpp
```

### `tests/foundation/json/test_json_writer.cpp`

```
Small_JsonWriter.WritesACompactDocument | keep | 9 | json_writer: the compact writer puts no space between tokens; one document built and compared with its text | tests/foundation/json/test_json_writer.cpp; modules/foundation/json/include/SushiProfiler/json/json_writer.hpp
Small_JsonWriter.IndentsBlocksAndKeepsInlineArraysOnOneLine | split | 3 | json_writer: the pretty writer indents blocks; json_writer: the pretty writer keeps Inline arrays on one line; the name joins two layouts that can break apart | tests/foundation/json/test_json_writer.cpp; modules/foundation/json/include/SushiProfiler/json/json_writer.hpp
Small_JsonWriter.EscapesControlCharactersAndQuotes | split | 3 | json_writer: strings escape quotes and backslashes; json_writer: strings escape control characters; the name joins two escape rules that can break apart | tests/foundation/json/test_json_writer.cpp; modules/foundation/json/include/SushiProfiler/json/json_writer.hpp; modules/foundation/json/source/json_writer.cpp
Small_JsonWriter.AWholeRealStaysAReal | split | 3 | json_writer: a whole real keeps a point or an exponent; json_writer: other reals print in their shortest exact form; the four values cover two rules | tests/foundation/json/test_json_writer.cpp; modules/foundation/json/include/SushiProfiler/json/json_writer.hpp; modules/foundation/json/source/json_writer.cpp
Small_JsonWriter.ANonFiniteRealIsNull | keep | 9 | json_writer: a non-finite real writes null; json_writer.cpp line 161 writes null for a non-finite value | tests/foundation/json/test_json_writer.cpp; modules/foundation/json/include/SushiProfiler/json/json_writer.hpp; modules/foundation/json/source/json_writer.cpp
Small_JsonWriter.AValueRoundTrips | keep | 9 | json_writer: a value written and read back is equal; the oracle is the first parse, a round trip through read_json | tests/foundation/json/test_json_writer.cpp; modules/foundation/json/include/SushiProfiler/json/json_writer.hpp; modules/foundation/json/include/SushiProfiler/json/json_reader.hpp
```

### `tests/foundation/version/test_export.cpp`

```
Small_Export.TheSharedLibraryConsumerImportsItsSymbols | delete | 1 | the test checks the expansion text of a macro, a constant, a forbidden kind; the promise that symbols are exported and imported is tested elsewhere: every test program links the library and calls exported classes such as Collector, so a missing export fails the link; cmake/SushiProfilerLibrary.cmake sets SUSHIPROFILER_EXPORTS on the shared target and SUSHIPROFILER_STATIC on the static one | tests/foundation/version/test_export.cpp; modules/foundation/version/include/SushiProfiler/version/export.hpp; cmake/SushiProfilerLibrary.cmake; tests/capture/zones/test_zone.cpp
```

### `tests/foundation/version/test_version.cpp`

```
Small_Version.StringIsTheDottedNumbers | rewrite | 1 | version: K_VERSION_STRING is the dotted K_VERSION numbers; the check is a version string, a forbidden kind; the promise is real and no other test holds it, since tests/package/consumer.cpp compares the string with the CMake project line, not with the numbers; rewrite against that promise | tests/foundation/version/test_version.cpp; modules/foundation/version/include/SushiProfiler/version/version.hpp; tests/package/consumer.cpp; tests/package/CMakeLists.txt
```

### `tests/package/consumer.cpp`

```
Small_Package.ReadsTheVersionThroughTheTarget | resize | 8 | package: an installed package is found by find_package and reports the version the tree builds; it loads an installed library file, which Small_ forbids; resize to Medium_. The label small is set by hand in tests/package/CMakeLists.txt, outside the module macro, and does not match the size | tests/package/consumer.cpp; tests/package/CMakeLists.txt; modules/foundation/version/include/SushiProfiler/version/version.hpp
```

### `tests/record/analysis/test_analysis.cpp`

```
Small_Analysis.OptionsDefaultToTheDesignedValues | delete | 1 | forbidden kind: a default value; no promise beyond the designed values, which the tests passing {} use (test_top_spans_analysis.cpp) | tests/record/analysis/test_analysis.cpp; tests/record/analysis/test_top_spans_analysis.cpp; modules/record/analysis/include/SushiProfiler/analysis/analysis.hpp
Small_Analysis.RunsThroughTheInterface | delete | 1 | forbidden kind: stub of the owned Analysis interface (PeriodCount); the runner calls catalog analyses through the same interface and tests/record/analysis/test_analysis_runner.cpp covers it | tests/record/analysis/test_analysis.cpp; tests/record/analysis/test_analysis_runner.cpp; modules/record/analysis/source/analysis_runner.cpp; modules/record/analysis/include/SushiProfiler/analysis/analysis.hpp
```

### `tests/record/analysis/test_analysis_catalog.cpp`

```
Small_AnalysisCatalog.MakesWhatItKnowsAndNothingElse | keep | 9 | catalog: a known name makes its analysis, an unknown name makes none, and knows reports which | tests/record/analysis/test_analysis_catalog.cpp; modules/record/analysis/include/SushiProfiler/analysis/analysis_catalog.hpp
Small_AnalysisCatalog.RefusesATakenName | keep | 9 | catalog: a taken name is refused | tests/record/analysis/test_analysis_catalog.cpp; modules/record/analysis/include/SushiProfiler/analysis/analysis_catalog.hpp
Small_AnalysisCatalog.TheBuiltinsAreListedInOrder | rewrite | 2 | catalog: names() lists the known names in ascending order, whatever their insertion order. The built-ins are added in alphabetical order, so a listing in insertion order passes too; no other test covers the ordering | tests/record/analysis/test_analysis_catalog.cpp; modules/record/analysis/include/SushiProfiler/analysis/analysis_catalog.hpp; modules/record/analysis/source/builtin_analyses.cpp
```

### `tests/record/analysis/test_analysis_runner.cpp`

```
Small_AnalysisRunner.RunsTheNamedAnalysesInTheOrderAsked | keep | 9 | analysis_runner: one table per named analysis, in the order asked | tests/record/analysis/test_analysis_runner.cpp; tests/record/analysis/analysis_record.hpp; modules/record/analysis/include/SushiProfiler/analysis/analysis_runner.hpp; modules/record/analysis/source/analysis_runner.cpp
Small_AnalysisRunner.AnUnknownNameRefusesTheWholeRun | keep | 9 | analysis_runner: an unknown name refuses the whole run and names the known analyses | tests/record/analysis/test_analysis_runner.cpp; tests/record/analysis/analysis_record.hpp; modules/record/analysis/include/SushiProfiler/analysis/analysis_runner.hpp; modules/record/analysis/source/analysis_runner.cpp
```

### `tests/record/analysis/test_breakdown_analysis.cpp`

```
Small_BreakdownAnalysis.ReportsSelfTimePerGroupThenTheUncoveredTime | keep | 9 | breakdown: one row per group with its self time per period, then one uncovered row, per timeline | tests/record/analysis/test_breakdown_analysis.cpp; tests/record/analysis/analysis_record.hpp; tests/record/analysis/analysis_record.cpp; modules/record/analysis/include/SushiProfiler/analysis/breakdown_analysis.hpp
```

### `tests/record/analysis/test_budget_analysis.cpp`

```
Small_BudgetAnalysis.FindsThePeriodsOverBudgetAndTheWorst | keep | 9 | budget: used fraction, periods over budget and worst period, per budgeted timeline | tests/record/analysis/test_budget_analysis.cpp; tests/record/analysis/analysis_record.hpp; tests/record/analysis/analysis_record.cpp; modules/record/analysis/include/SushiProfiler/analysis/budget_analysis.hpp
Small_BudgetAnalysis.ATimelineWithoutABudgetHasNoRow | rewrite | 1 | budget: a timeline without a budget has no row. Forbidden kind: control flow in the body (a for loop clears the budgets); a builder named for the record holds it. Only test of this promise | tests/record/analysis/test_budget_analysis.cpp; tests/record/analysis/analysis_record.cpp; modules/record/analysis/include/SushiProfiler/analysis/budget_analysis.hpp
```

### `tests/record/analysis/test_counters_analysis.cpp`

```
Small_CountersAnalysis.FoldsEachCounterByItsOperation | keep | 9 | counters: each counter folds per active period by its operation, then is summarised | tests/record/analysis/test_counters_analysis.cpp; tests/record/analysis/analysis_record.hpp; tests/record/analysis/analysis_record.cpp; modules/record/analysis/include/SushiProfiler/analysis/counters_analysis.hpp
```

### `tests/record/analysis/test_device_idle_analysis.cpp`

```
Small_DeviceIdleAnalysis.SplitsEachTracksIdleTimeByCause | keep | 9 | device_idle: idle time per track and active period, split by cause | tests/record/analysis/test_device_idle_analysis.cpp; tests/record/analysis/analysis_record.hpp; tests/record/analysis/analysis_record.cpp; modules/record/analysis/include/SushiProfiler/analysis/device_idle_analysis.hpp
Small_DeviceIdleAnalysis.NamesTheInnermostHostZoneOpenOnTheSubmittingThreadAtTheGapsStart | rewrite | 1 | device_idle: the blamed zone is the innermost zone open on the submitting thread at the gap's start. Forbidden kind: bare tolerance literal 1e-12 in EXPECT_NEAR; the value 0.75 is exact, so an exact or helper comparison fits. Only test of this promise | tests/record/analysis/test_device_idle_analysis.cpp; modules/record/analysis/include/SushiProfiler/analysis/device_idle_analysis.hpp
Small_DeviceIdleAnalysis.IgnoresAZoneOpenOnAnotherThread | rewrite | 1 | device_idle: a zone open on another thread is not blamed. Forbidden kind: bare tolerance literal 1e-12 in EXPECT_NEAR. Only test of this promise | tests/record/analysis/test_device_idle_analysis.cpp; modules/record/analysis/include/SushiProfiler/analysis/device_idle_analysis.hpp
Small_DeviceIdleAnalysis.ChargesAGapToLaunchWhenNoZoneIsOpenAndTheNextSpanWasQueued | rewrite | 1 | device_idle: a gap with no open zone whose next span was queued is charged to launch. Forbidden kind: bare tolerance literal 1e-12 in EXPECT_NEAR. Only test of this promise | tests/record/analysis/test_device_idle_analysis.cpp; modules/record/analysis/include/SushiProfiler/analysis/device_idle_analysis.hpp
Small_DeviceIdleAnalysis.ARecordWithoutSpansHasNoRows | keep | 9 | device_idle: a record without device spans has no rows | tests/record/analysis/test_device_idle_analysis.cpp; tests/record/analysis/analysis_record.hpp; tests/record/analysis/analysis_record.cpp; modules/record/analysis/include/SushiProfiler/analysis/device_idle_analysis.hpp
```

### `tests/record/analysis/test_distribution_analysis.cpp`

```
Small_DistributionAnalysis.ReportsEachTimelineZoneCounterAndValue | keep | 9 | distribution: one row per timeline, zone, counter and value with percentiles, jitter and log2 histogram | tests/record/analysis/test_distribution_analysis.cpp; tests/record/analysis/analysis_record.hpp; tests/record/analysis/analysis_record.cpp; modules/record/analysis/include/SushiProfiler/analysis/distribution_analysis.hpp
```

### `tests/record/analysis/test_flows_analysis.cpp`

```
Small_FlowsAnalysis.MeasuresTotalAndStepLatencyAcrossPeriods | keep | 9 | flows: total and step latency per flow, across periods | tests/record/analysis/test_flows_analysis.cpp; tests/record/analysis/analysis_record.hpp; tests/record/analysis/analysis_record.cpp; modules/record/analysis/include/SushiProfiler/analysis/flows_analysis.hpp
Small_FlowsAnalysis.JoinsTheCallSitesOfOneFlowByName | rewrite | 1 | flows: call sites of one flow with the same name join into one row. Forbidden kind: control flow in the body (loops and an if rewrite the fixture); a named builder holds it. Only test of this promise | tests/record/analysis/test_flows_analysis.cpp; tests/record/analysis/analysis_record.cpp; modules/record/analysis/include/SushiProfiler/analysis/flows_analysis.hpp
```

### `tests/record/analysis/test_overhead_analysis.cpp`

```
Small_OverheadAnalysis.CountsEventsAndDropsAndNotesAMissingBaseline | split | 3 | two promises in one name and body: the counts rows (events, bytes, dropped, late, unmatched, outside_periods, open_at_stop, spans_dropped, clock_read_ns), and the note for a missing baseline, which the test counts but never reads | tests/record/analysis/test_overhead_analysis.cpp; tests/record/analysis/analysis_record.hpp; tests/record/analysis/analysis_record.cpp; modules/record/analysis/include/SushiProfiler/analysis/overhead_analysis.hpp
Small_OverheadAnalysis.MeasuresTheCostAgainstABaseline | rewrite | 1 | overhead: against a baseline, the cost is the ratio of period-time medians (0.1 here). Forbidden kind: control flow in the body (a loop builds the baseline); a builder holds it. Only test of this promise | tests/record/analysis/test_overhead_analysis.cpp; tests/record/analysis/analysis_record.cpp; modules/record/analysis/include/SushiProfiler/analysis/overhead_analysis.hpp
```

### `tests/record/analysis/test_period_breakdown.cpp`

```
Small_PeriodBreakdown.InheritsTheNearestGroupedAncestorsGroup | keep | 9 | period_breakdown: an ungrouped zone takes its nearest grouped ancestor's group | tests/record/analysis/test_period_breakdown.cpp; modules/record/analysis/include/SushiProfiler/analysis/period_breakdown.hpp
Small_PeriodBreakdown.ChargesUngroupedZonesToTheUngroupedLabel | keep | 9 | period_breakdown: an ungrouped zone with no grouped ancestor falls under the ungrouped label | tests/record/analysis/test_period_breakdown.cpp; modules/record/analysis/include/SushiProfiler/analysis/period_breakdown.hpp
Small_PeriodBreakdown.OrdersGroupsByDescendingSelfTime | keep | 9 | period_breakdown: groups are ordered by descending self time (the tie-break by name is an add) | tests/record/analysis/test_period_breakdown.cpp; modules/record/analysis/include/SushiProfiler/analysis/period_breakdown.hpp
Small_PeriodBreakdown.CountsOnlyTheMarkingThreadsRootsAsCovered | keep | 9 | period_breakdown: only the marking thread's roots count as covered time | tests/record/analysis/test_period_breakdown.cpp; modules/record/analysis/include/SushiProfiler/analysis/period_breakdown.hpp
Small_PeriodBreakdown.UncoveredIsZeroWhenZonesOverrunThePeriod | keep | 9 | period_breakdown: uncovered time is never below zero | tests/record/analysis/test_period_breakdown.cpp; modules/record/analysis/include/SushiProfiler/analysis/period_breakdown.hpp
Small_PeriodBreakdown.ReportsEveryThreadsTotalsWithLanesSummed | keep | 9 | period_breakdown: every thread's root and self time, lanes summed, by ascending thread index | tests/record/analysis/test_period_breakdown.cpp; modules/record/analysis/include/SushiProfiler/analysis/period_breakdown.hpp
Small_PeriodBreakdown.MatchesTheBreakdownAnalysisOnOnePeriod | delete | 1 | forbidden kind: control flow in the body (loops and an if); the oracle is break_down_period, the code under test (question 6). Its two sides are each pinned by their own unit's tests (test_breakdown_analysis.cpp, the period tests above), so the cross-check states no promise of its own | tests/record/analysis/test_period_breakdown.cpp; tests/record/analysis/test_breakdown_analysis.cpp; modules/record/analysis/include/SushiProfiler/analysis/period_breakdown.hpp; modules/record/analysis/include/SushiProfiler/analysis/breakdown_analysis.hpp
```

### `tests/record/analysis/test_roofline_analysis.cpp`

```
Small_RooflineAnalysis.GivesEachNodeItsRowAndSplitsAMixedTypeAndMergesTheSameOne | keep | 9 | roofline: rows are keyed by node and dtype; equal keys share a row, a node with two types splits | tests/record/analysis/test_roofline_analysis.cpp; tests/record/analysis/analysis_record.hpp; modules/record/analysis/include/SushiProfiler/analysis/roofline_analysis.hpp
Small_RooflineAnalysis.PicksTheCeilingOfTheSpansTypeAndFallsBackToTheTypelessOne | keep | 9 | roofline: the ceiling is the spans' dtype peak when the environment states one, else the device peak | tests/record/analysis/test_roofline_analysis.cpp; tests/record/analysis/analysis_record.hpp; modules/record/analysis/include/SushiProfiler/analysis/roofline_analysis.hpp
Small_RooflineAnalysis.ARowWithNoFlopsButBytesIsMeasuredAgainstTheMemoryCeiling | keep | 9 | roofline: a row with no flops is limited by the memory ceiling | tests/record/analysis/test_roofline_analysis.cpp; tests/record/analysis/analysis_record.hpp; modules/record/analysis/include/SushiProfiler/analysis/roofline_analysis.hpp
Small_RooflineAnalysis.ReportsAchievedRatesAndIntensityPerPeriodNameAndTrack | keep | 9 | roofline: achieved FLOP/s, bytes/s and intensity per period, name and track | tests/record/analysis/test_roofline_analysis.cpp; tests/record/analysis/analysis_record.hpp; modules/record/analysis/include/SushiProfiler/analysis/roofline_analysis.hpp
Small_RooflineAnalysis.LeavesTheCeilingCellsEmptyWhenTheTrackHasNoFacts | rewrite | 1 | roofline: ceiling cells are empty when the track states no facts. Forbidden kinds: control flow in the body (a loop over columns) and bare tolerance literals 1.0 and 1e-9 in EXPECT_NEAR. Only test of this promise; use expect_row on the whole row | tests/record/analysis/test_roofline_analysis.cpp; modules/record/analysis/include/SushiProfiler/analysis/roofline_analysis.hpp
Small_RooflineAnalysis.CountsSpansWithoutBothAttributesAndSkipsWarmupPeriods | rewrite | 1 | roofline: spans without both cost attributes are counted in one note; warm-up periods are skipped. Forbidden kind: control flow in the body (a loop checks every row); the name holds two promises. Only test of the note | tests/record/analysis/test_roofline_analysis.cpp; modules/record/analysis/include/SushiProfiler/analysis/roofline_analysis.hpp
Small_RooflineAnalysis.ARecordWithoutCostAttributesHasNoRowsAndCountsEverySpan | rewrite | 7 | roofline: a record with no cost attributes has no rows; the note counts its non-warm-up spans. The name says every span is counted, but the body expects 5 where span_record holds 6 spans, the warm-up span excluded; name and body disagree | tests/record/analysis/test_roofline_analysis.cpp; tests/record/analysis/analysis_record.cpp; modules/record/analysis/include/SushiProfiler/analysis/roofline_analysis.hpp
Small_RooflineAnalysis.AnUnreadableCeilingFactIsTreatedAsMissing | keep | 9 | roofline: an unreadable ceiling fact leaves its bound cell empty | tests/record/analysis/test_roofline_analysis.cpp; modules/record/analysis/include/SushiProfiler/analysis/roofline_analysis.hpp
Small_RooflineAnalysis.TwoGraphsWithTheSameNodeIndexGiveTwoRows | keep | 9 | roofline: the same node index in two graphs gives two rows | tests/record/analysis/test_roofline_analysis.cpp; modules/record/analysis/include/SushiProfiler/analysis/roofline_analysis.hpp
Small_RooflineAnalysis.TwoOriginsOfOneGraphGiveTwoRowsAndTheSameOriginMerges | keep | 9 | roofline: rows are keyed by graph and origin; equal origins of one graph share a row | tests/record/analysis/test_roofline_analysis.cpp; modules/record/analysis/include/SushiProfiler/analysis/roofline_analysis.hpp
Small_RooflineAnalysis.SpansWithoutGraphOrOriginKeepTheirRowAndValues | keep | 9 | roofline: spans without graph or origin keep their row and values | tests/record/analysis/test_roofline_analysis.cpp; modules/record/analysis/include/SushiProfiler/analysis/roofline_analysis.hpp
```

### `tests/record/analysis/test_sample_summary.cpp`

```
Small_SampleSummary.SummarisesFixedSamples | rewrite | 1 | sample_summary: median, bootstrap interval, minimum, p95 and cv of fixed samples. Forbidden kind: bare tolerance literal 1e-12 in EXPECT_NEAR on cv. Only test of this promise | tests/record/analysis/test_sample_summary.cpp; modules/record/analysis/include/SushiProfiler/analysis/sample_summary.hpp
Small_SampleSummary.NoSamplesGiveZeros | rewrite | 2 | sample_summary: with no samples every field but count is 0. The test asserts count, median and high; the header promises every field, so a break in low, minimum, p95 or cv still passes. Only test of this promise | tests/record/analysis/test_sample_summary.cpp; modules/record/analysis/include/SushiProfiler/analysis/sample_summary.hpp
Small_SampleSummary.ColumnsAndCellsLineUp | keep | 9 | sample_summary: columns and cells line up in order with their units | tests/record/analysis/test_sample_summary.cpp; modules/record/analysis/include/SushiProfiler/analysis/sample_summary.hpp
```

### `tests/record/analysis/test_short_spans_analysis.cpp`

```
Small_ShortSpansAnalysis.CountsTheSpansUnderTheBound | keep | 9 | short_spans: spans under the bound are counted, with their total time and share | tests/record/analysis/test_short_spans_analysis.cpp; tests/record/analysis/analysis_record.hpp; tests/record/analysis/analysis_record.cpp; modules/record/analysis/include/SushiProfiler/analysis/short_spans_analysis.hpp
Small_ShortSpansAnalysis.PricesShortSpansAtTheLaunchFloor | keep | 9 | short_spans: each short span is priced at the launch floor | tests/record/analysis/test_short_spans_analysis.cpp; tests/record/analysis/analysis_record.hpp; tests/record/analysis/analysis_record.cpp; modules/record/analysis/include/SushiProfiler/analysis/short_spans_analysis.hpp
```

### `tests/record/analysis/test_subject_samples.cpp`

```
Small_SubjectSamples.ListsEveryHostSubjectInOrder | keep | 9 | subject_samples: every subject, in fixed order, with its samples | tests/record/analysis/test_subject_samples.cpp; tests/record/analysis/analysis_record.hpp; tests/record/analysis/analysis_record.cpp; modules/record/analysis/include/SushiProfiler/analysis/subject_samples.hpp
Small_SubjectSamples.ListsSpansByName | keep | 9 | subject_samples: span subjects are listed by name, warm-up excluded | tests/record/analysis/test_subject_samples.cpp; tests/record/analysis/analysis_record.hpp; tests/record/analysis/analysis_record.cpp; modules/record/analysis/include/SushiProfiler/analysis/subject_samples.hpp
```

### `tests/record/analysis/test_table.cpp`

```
Small_Table.ACellHoldsOneKind | keep | 9 | table: a cell holds one kind, and accessors return null for the others | tests/record/analysis/test_table.cpp; modules/record/analysis/include/SushiProfiler/analysis/table.hpp
Small_Table.CellsOfDifferentKindsDiffer | keep | 9 | table: cells of different kinds are unequal | tests/record/analysis/test_table.cpp; modules/record/analysis/include/SushiProfiler/analysis/table.hpp
Small_Table.FindsAColumnByName | keep | 9 | table: a column is found by name; an unknown name gives the column count | tests/record/analysis/test_table.cpp; modules/record/analysis/include/SushiProfiler/analysis/table.hpp
```

### `tests/record/analysis/test_top_spans_analysis.cpp`

```
Small_TopSpansAnalysis.RanksSpansByNameAndTotalTime | keep | 9 | top_spans: spans grouped by name, ranked by total time, with median and share | tests/record/analysis/test_top_spans_analysis.cpp; tests/record/analysis/analysis_record.hpp; tests/record/analysis/analysis_record.cpp; modules/record/analysis/include/SushiProfiler/analysis/top_spans_analysis.hpp
Small_TopSpansAnalysis.GroupsByAnAttributeAndKeepsTheTop | keep | 9 | top_spans: spans grouped by the named attribute, top kept, the rest noted | tests/record/analysis/test_top_spans_analysis.cpp; tests/record/analysis/analysis_record.hpp; tests/record/analysis/analysis_record.cpp; modules/record/analysis/include/SushiProfiler/analysis/top_spans_analysis.hpp
```

### `tests/record/checks/test_check.cpp`

```
Small_Check.ReadsTimeAndPlainChecks | keep | 9 | checks: time bounds read as at_most_ns and at_least_ns, plain bounds as at_most and at_least | tests/record/checks/test_check.cpp; modules/record/checks/include/SushiProfiler/checks/check.hpp; modules/record/checks/source/check.cpp
Small_Check.NamesEveryStatistic | delete | 1 | forbidden kind: trivial code, a constant lookup into K_STATISTICS; it names three of seven statistics. The words users see are checked by the verdict reasons (test_verdict.cpp) and the refusal texts (test_check.cpp) | tests/record/checks/test_check.cpp; tests/record/checks/test_verdict.cpp; modules/record/checks/include/SushiProfiler/checks/check.hpp; modules/record/checks/source/check.cpp
Small_Check.RefusesATimeBoundOnAPlainSubjectAndTheReverse | keep | 9 | checks: a time bound on a plain subject, and the reverse, is refused with the key that fits | tests/record/checks/test_check.cpp; modules/record/checks/include/SushiProfiler/checks/check.hpp; modules/record/checks/source/check.cpp
Small_Check.RefusesWhatTheFormatDoesNotHold | split | 3 | seven refusals, each a different rule (shape, subject prefix, statistic, missing key, missing bound, number type, JSON syntax); each rule is one promise, so each is one test | tests/record/checks/test_check.cpp; modules/record/checks/include/SushiProfiler/checks/check.hpp; modules/record/checks/source/check.cpp
```

### `tests/record/checks/test_verdict.cpp`

```
Small_Verdict.PassesWithinTheBounds | keep | 9 | verdict: a value within both bounds passes with no reason | tests/record/checks/test_verdict.cpp; modules/record/checks/include/SushiProfiler/checks/verdict.hpp; modules/record/checks/source/verdict.cpp
Small_Verdict.FailsAboveAtMostAndBelowAtLeast | keep | 9 | verdict: a value above at_most or below at_least fails with a reason naming the bound | tests/record/checks/test_verdict.cpp; modules/record/checks/include/SushiProfiler/checks/verdict.hpp; modules/record/checks/source/verdict.cpp
Small_Verdict.PercentilesFollowTheType7Rule | rewrite | 1 | verdict: percentiles interpolate by the type-7 rule. Forbidden kind: bare tolerance literal 1e-9 in EXPECT_NEAR. Only test of this promise | tests/record/checks/test_verdict.cpp; modules/record/checks/include/SushiProfiler/checks/verdict.hpp
Small_Verdict.AMissingSubjectFails | keep | 9 | verdict: a check on a subject with no samples fails with a reason | tests/record/checks/test_verdict.cpp; modules/record/checks/include/SushiProfiler/checks/verdict.hpp; modules/record/checks/source/verdict.cpp
```

### `tests/record/compare/test_compare_records.cpp`

```
Small_CompareRecords.NamesAChangeWhenTheIntervalsAreApart | keep | 9 | compare: non-overlapping intervals name faster or slower by the medians; the row holds the medians and their ratio | tests/record/compare/test_compare_records.cpp; modules/record/compare/include/SushiProfiler/compare/compare_records.hpp
Small_CompareRecords.OverlappingIntervalsAreNoChange | rewrite | 1 | compare: overlapping intervals name no change; the ratio of medians is reported. Forbidden kind: bare tolerance literal 1e-12 in EXPECT_NEAR on the ratio. Only test of this promise | tests/record/compare/test_compare_records.cpp; modules/record/compare/include/SushiProfiler/compare/compare_records.hpp
Small_CompareRecords.ASlowerCandidateIsNamed | merge | 4 | merge with tests/record/compare/test_compare_records.cpp::Small_CompareRecords.NamesAChangeWhenTheIntervalsAreApart: the same promise (a change is named by the medians) at the other inputs (slower); one parametrised test holds both | tests/record/compare/test_compare_records.cpp; modules/record/compare/include/SushiProfiler/compare/compare_records.hpp
Small_CompareRecords.RefusesDifferingBuildsUnlessAllowed | split | 3 | two actions (the refused call, and the call with allow_build_mismatch); each is one promise: the refusal names the field and both values, and an allowed mismatch is noted | tests/record/compare/test_compare_records.cpp; modules/record/compare/include/SushiProfiler/compare/compare_records.hpp
Small_CompareRecords.GroupsRowsByTagsWhenBothRecordsCarryThem | keep | 9 | compare: rows are grouped by tag set when both records carry tags | tests/record/compare/test_compare_records.cpp; modules/record/compare/include/SushiProfiler/compare/compare_records.hpp
Small_CompareRecords.ASubjectInOneRecordOnlyHasNoRow | keep | 9 | compare: a subject in one record only has no row | tests/record/compare/test_compare_records.cpp; modules/record/compare/include/SushiProfiler/compare/compare_records.hpp
```

### `tests/record/record/test_device_span_keys.cpp`

```
Small_DeviceSpanKeys.TheAttributeKeysAreTheWordsTheSourcesWrite | rewrite | 1 | device_span_keys key spellings: a constant check (forbidden kind); the promise is real and the spelling is only tested here, so rewrite as a write-then-read round trip with roofline | tests/record/record/test_device_span_keys.cpp; modules/record/record/include/SushiProfiler/record/device_span_keys.hpp; modules/record/analysis/source/roofline_analysis.cpp; tests/record/analysis/test_roofline_analysis.cpp
Small_DeviceSpanKeys.TheCeilingFactsNameTheTrack | keep | 9 | device_span_keys: ceiling fact is device.<track>.peak_flops and .peak_bytes_per_s | tests/record/record/test_device_span_keys.cpp; modules/record/record/include/SushiProfiler/record/device_span_keys.hpp; modules/record/record/source/device_span_keys.cpp; tests/record/analysis/test_roofline_analysis.cpp
Small_DeviceSpanKeys.ThePerTypeComputeCeilingAppendsTheType | keep | 9 | device_span_keys: ceiling fact with .<dtype> | tests/record/record/test_device_span_keys.cpp; modules/record/record/include/SushiProfiler/record/device_span_keys.hpp; modules/record/record/source/device_span_keys.cpp
Small_DeviceSpanKeys.AnEmptyTrackLeavesTheSeparatorsInPlace | keep | 9 | device_span_keys: ceiling fact for an empty track (edge of the format) | tests/record/record/test_device_span_keys.cpp; modules/record/record/include/SushiProfiler/record/device_span_keys.hpp; modules/record/record/source/device_span_keys.cpp
```

### `tests/record/record/test_period_record.cpp`

```
Small_PeriodRecord.DurationIsEndLessBegin | keep | 9 | period_record: duration is end minus begin (a computed length, not a getter) | tests/record/record/test_period_record.cpp; modules/record/record/include/SushiProfiler/record/period_record.hpp
Small_PeriodRecord.EqualityReachesTreesTagsAndSpans | split | 3 | period_record: equality compares zone trees, tags and spans; the name states three promises and the body mutates three fields | tests/record/record/test_period_record.cpp; modules/record/record/include/SushiProfiler/record/period_record.hpp; modules/record/record/source/period_record.cpp
Small_PeriodRecord.EqualityReachesEveryFieldOfAZoneRun | rewrite | 2 | period_record: ZoneRun equality reaches every field; the test changes depth and begin only, so a dropped lane, site, name, thread or end passes; no other test covers it | tests/record/record/test_period_record.cpp; modules/record/record/include/SushiProfiler/record/period_record.hpp; modules/record/record/source/period_record.cpp
```

### `tests/record/record/test_run_record.cpp`

```
Small_RunRecord.NamesAndSitesAreLookedUpById | split | 3 | run_record: name_of and site are two lookups with two promises; the assertions compare different calls | tests/record/record/test_run_record.cpp; modules/record/record/include/SushiProfiler/record/run_record.hpp; modules/record/record/source/run_record.cpp
Small_RunRecord.FindNameReturnsTheIdOrZero | keep | 9 | run_record: find_name returns the id or 0 | tests/record/record/test_run_record.cpp; modules/record/record/include/SushiProfiler/record/run_record.hpp; modules/record/record/source/run_record.cpp
Small_RunRecord.EqualityReachesEveryTable | rewrite | 2 | run_record: equality reaches every table; the name says every table, the body changes sites, drops and clock; a mutant dropping tracks, threads or periods from operator== survives; no other test covers it | tests/record/record/test_run_record.cpp; modules/record/record/include/SushiProfiler/record/run_record.hpp; modules/record/record/source/run_record.cpp
```

### `tests/record/record/test_sample_series.cpp`

```
Small_SampleSeries.EqualityComparesEveryColumn | rewrite | 2 | sample_series: equality compares every column; only points is changed, so a dropped values, time or items column from operator== survives; no other test covers it | tests/record/record/test_sample_series.cpp; modules/record/record/include/SushiProfiler/record/sample_series.hpp; modules/record/record/source/sample_series.cpp
```

### `tests/record/record/test_span_table.cpp`

```
Small_SpanTable.AppendsSpansAndReadsThemBack | rewrite | 2 | span_table: span() reads back what append stored; only queued_ns and end_ns are checked, so a span() that drops start_ns, track, name, submit_thread or uncertainty passes | tests/record/record/test_span_table.cpp; modules/record/record/include/SushiProfiler/record/span_table.hpp; modules/record/record/source/span_table.cpp
Small_SpanTable.AColumnAddedLateMarksEarlierSpansAbsent | keep | 9 | span_table: a column added late marks earlier spans absent | tests/record/record/test_span_table.cpp; modules/record/record/include/SushiProfiler/record/span_table.hpp; modules/record/record/source/span_table.cpp
Small_SpanTable.ASpanAddedLateIsAbsentFromEveryColumn | rewrite | 7 | span_table: a late span is absent from every column; the body loops over the columns in an assertion loop, which the straight-line rule forbids; rows or one check per column | tests/record/record/test_span_table.cpp; modules/record/record/include/SushiProfiler/record/span_table.hpp; modules/record/record/source/span_table.cpp
Small_SpanTable.EqualTablesCompareEqual | rewrite | 2 | span_table: equal tables compare equal; only one attribute value is changed, so SpanTable::operator== ignoring track, name or any span column survives; no other test covers it | tests/record/record/test_span_table.cpp; modules/record/record/include/SushiProfiler/record/span_table.hpp; modules/record/record/source/span_table.cpp
```

### `tests/record/record_json/test_record_json.cpp`

```
Small_RecordJson.AFullRecordRoundTripsByteForByte | keep | 9 | record_json: round trip to an equal record and the same bytes | tests/record/record_json/test_record_json.cpp; modules/record/record_json/include/SushiProfiler/record_json/record_json.hpp; modules/record/record_json/README.md
Small_RecordJson.AnEmptyRecordRoundTrips | merge | 4 | record_json: round trip, other input (empty record); merge into rows with AFullRecordRoundTripsByteForByte | tests/record/record_json/test_record_json.cpp; modules/record/record_json/include/SushiProfiler/record_json/record_json.hpp
Small_RecordJson.ColumnsSitOnOneLine | keep | 9 | record_json: columns sit on one line (documented in the header and the module README) | tests/record/record_json/test_record_json.cpp; modules/record/record_json/include/SushiProfiler/record_json/record_json.hpp; modules/record/record_json/README.md
Small_RecordJson.NonFiniteRealsAreWrittenAsWords | keep | 9 | record_json: non-finite reals written as words and read back | tests/record/record_json/test_record_json.cpp; modules/record/record_json/include/SushiProfiler/record_json/record_json.hpp
Small_RecordJson.ANewerVersionIsRefused | merge | 4 | record_json: version refusal with a version other than 2; merge into rows with TheVersionBeforeZoneRunsIsRefusedByItsNumber | tests/record/record_json/test_record_json.cpp; modules/record/record_json/include/SushiProfiler/record_json/record_json.hpp
Small_RecordJson.TheVersionBeforeZoneRunsIsRefusedByItsNumber | merge | 4 | record_json: version refusal with version 1; merge into rows with ANewerVersionIsRefused | tests/record/record_json/test_record_json.cpp; modules/record/record_json/include/SushiProfiler/record_json/record_json.hpp
Small_RecordJson.AnUnknownKeyIsRefusedWithItsPath | keep | 9 | record_json: unknown key refused with its path | tests/record/record_json/test_record_json.cpp; modules/record/record_json/include/SushiProfiler/record_json/record_json.hpp
Small_RecordJson.AMissingKeyIsRefusedWithItsPath | keep | 9 | record_json: missing key refused with its path | tests/record/record_json/test_record_json.cpp; modules/record/record_json/include/SushiProfiler/record_json/record_json.hpp
Small_RecordJson.AValueOutOfRangeIsRefusedWithItsPath | keep | 9 | record_json: out-of-range value refused with its path | tests/record/record_json/test_record_json.cpp; modules/record/record_json/include/SushiProfiler/record_json/record_json.hpp
Small_RecordJson.AValueOfTheWrongKindIsRefusedWithItsPath | keep | 9 | record_json: wrong-kind value refused with its path | tests/record/record_json/test_record_json.cpp; modules/record/record_json/include/SushiProfiler/record_json/record_json.hpp
Small_RecordJson.ColumnsOfUnequalLengthAreRefused | keep | 9 | record_json: columns of unequal length refused | tests/record/record_json/test_record_json.cpp; modules/record/record_json/include/SushiProfiler/record_json/record_json.hpp
Small_RecordJson.ASiteThatPointsNowhereIsRefused | merge | 4 | record_json: dangling site id refused in zone nodes; merge into rows with AZoneRunOnASiteThatPointsNowhereIsRefused | tests/record/record_json/test_record_json.cpp; modules/record/record_json/include/SushiProfiler/record_json/record_json.hpp
Small_RecordJson.AZoneRunOnASiteThatPointsNowhereIsRefused | merge | 4 | record_json: dangling site id refused in zone runs; merge into rows with ASiteThatPointsNowhereIsRefused | tests/record/record_json/test_record_json.cpp; modules/record/record_json/include/SushiProfiler/record_json/record_json.hpp
Small_RecordJson.AZoneRunThatEndsBeforeItBeginsIsRefused | keep | 9 | record_json: zone run ending before it begins refused with its path (the header's refusal list does not name this case; the module README does not either, a doc gap) | tests/record/record_json/test_record_json.cpp; modules/record/record_json/include/SushiProfiler/record_json/record_json.hpp; modules/record/record_json/README.md
Small_RecordJson.AZoneRunOnAThreadOutsideTheTableIsRefused | merge | 4 | record_json: thread id outside the table refused in zone runs; merge into rows with AThreadOutsideTheTableIsRefused | tests/record/record_json/test_record_json.cpp; modules/record/record_json/include/SushiProfiler/record_json/record_json.hpp
Small_RecordJson.AParentThatIsNotEarlierIsRefused | keep | 9 | record_json: parent that is not earlier refused with its path | tests/record/record_json/test_record_json.cpp; modules/record/record_json/include/SushiProfiler/record_json/record_json.hpp
Small_RecordJson.AThreadOutsideTheTableIsRefused | merge | 4 | record_json: thread id outside the table refused in a period; merge into rows with AZoneRunOnAThreadOutsideTheTableIsRefused | tests/record/record_json/test_record_json.cpp; modules/record/record_json/include/SushiProfiler/record_json/record_json.hpp
Small_RecordJson.AnUnknownWordIsRefused | keep | 9 | record_json: unknown word refused with its path | tests/record/record_json/test_record_json.cpp; modules/record/record_json/include/SushiProfiler/record_json/record_json.hpp
Small_RecordJson.TextThatIsNotJsonNamesItsPlace | keep | 9 | record_json: text that is not JSON names line and column | tests/record/record_json/test_record_json.cpp; modules/record/record_json/include/SushiProfiler/record_json/record_json.hpp
```

### `tests/record/record_writers/test_record_file_writer.cpp`

```
Medium_RecordFileWriter.SavesARecordThatReadsBack | keep | 9 | record_file_writer: saved text is write_record's text and reads back | tests/record/record_writers/test_record_file_writer.cpp; modules/record/record_writers/include/SushiProfiler/record_writers/record_file_writer.hpp; modules/record/record_writers/include/SushiProfiler/record_writers/record_writer.hpp
Medium_RecordFileWriter.NamesAPathItCannotOpen | keep | 9 | record_file_writer: unopenable path gives PathUnwritable (a file boundary, medium is the right size) | tests/record/record_writers/test_record_file_writer.cpp; modules/record/record_writers/include/SushiProfiler/record_writers/record_file_writer.hpp; modules/record/record_writers/include/SushiProfiler/record_writers/record_writer.hpp
```

### `tests/record/record_writers/test_record_writer.cpp`

```
Small_RecordWriter.WritersRunThroughTheInterface | rewrite | 1 | record_writer: the CountingWriter is a spy of the owned RecordWriter interface, made by the test (forbidden kind); a loop over writers sits in the body; rewrite against the real TerminalWriter and RecordFileWriter as rows | tests/record/record_writers/test_record_writer.cpp; modules/record/record_writers/include/SushiProfiler/record_writers/record_writer.hpp; modules/record/record_writers/include/SushiProfiler/record_writers/terminal_writer.hpp
```

### `tests/record/record_writers/test_terminal_writer.cpp`

```
Small_TerminalWriter.PrintsAlignedTablesAndCountsTheRowsCut | split | 3 | terminal_writer: aligned tables, the row cut count, the note line and (no rows) are four promises in one golden text | tests/record/record_writers/test_terminal_writer.cpp; modules/record/record_writers/include/SushiProfiler/record_writers/terminal_writer.hpp
Small_TerminalWriter.WidensColumnsForEveryPrintedRow | keep | 9 | terminal_writer: columns widen to the widest printed cell | tests/record/record_writers/test_terminal_writer.cpp; modules/record/record_writers/include/SushiProfiler/record_writers/terminal_writer.hpp
Small_TerminalWriter.FormatsTimesInReadableSteps | merge | 4 | terminal_writer: cell format per unit, same promise as FormatsRatiosCountsAndPlainNumbers; merge into rows | tests/record/record_writers/test_terminal_writer.cpp; modules/record/record_writers/include/SushiProfiler/record_writers/terminal_writer.hpp
Small_TerminalWriter.FormatsRatiosCountsAndPlainNumbers | merge | 4 | terminal_writer: cell format per unit; merge into rows with FormatsTimesInReadableSteps | tests/record/record_writers/test_terminal_writer.cpp; modules/record/record_writers/include/SushiProfiler/record_writers/terminal_writer.hpp
```

### `tests/record/statistics/test_bootstrap.cpp`

```
Small_Bootstrap.TheSameSeedGivesTheSameInterval | keep | 9 | bootstrap: same samples and seed give the same interval | tests/record/statistics/test_bootstrap.cpp; modules/record/statistics/include/SushiProfiler/statistics/bootstrap.hpp; modules/record/statistics/source/bootstrap.cpp
Small_Bootstrap.TheDefaultSeedGivesThePinnedInterval | characterize | 6 | bootstrap: pinned default-seed ends; the oracle is a Python reimplementation that is not in the repository and cannot be rerun here, so the value is pasted from output; rename the suite Characterization_ and open a backlog entry | tests/record/statistics/test_bootstrap.cpp; modules/record/statistics/include/SushiProfiler/statistics/bootstrap.hpp; modules/record/statistics/source/bootstrap.cpp
Small_Bootstrap.TheIntervalHoldsTheMedianAndIgnoresTheOutlier | keep | 9 | bootstrap: interval of the median ignores the outlier; bounds are loose (low <= 12 <= high < 50), so a mean-based interval may pass; the pinned default-seed test is the tight check; noted, not a verdict change | tests/record/statistics/test_bootstrap.cpp; modules/record/statistics/include/SushiProfiler/statistics/bootstrap.hpp; modules/record/statistics/source/bootstrap.cpp
Small_Bootstrap.OneSampleGivesAPointInterval | keep | 9 | bootstrap: one sample gives a point interval | tests/record/statistics/test_bootstrap.cpp; modules/record/statistics/include/SushiProfiler/statistics/bootstrap.hpp; modules/record/statistics/source/bootstrap.cpp
```

### `tests/record/statistics/test_dispersion.cpp`

```
Small_Dispersion.TheMeanOfNothingIsZero | split | 3 | dispersion: mean of nothing is 0 and the mean of values is their average; two promises | tests/record/statistics/test_dispersion.cpp; modules/record/statistics/include/SushiProfiler/statistics/dispersion.hpp; modules/record/statistics/source/dispersion.cpp
Small_Dispersion.TheCoefficientOfVariationUsesTheSampleDeviation | rewrite | 1 | dispersion: CV is the sample deviation over the mean; the comparison uses a bare tolerance literal (1e-15), a forbidden kind; use the tolerance helper or an exact case | tests/record/statistics/test_dispersion.cpp; modules/record/statistics/include/SushiProfiler/statistics/dispersion.hpp; modules/record/statistics/source/dispersion.cpp
Small_Dispersion.TheCoefficientOfVariationIsZeroWithoutASpread | split | 3 | dispersion: CV is 0 for one sample, for a mean of 0 and for no spread; the name covers only the last; the three zero rules are separate cases | tests/record/statistics/test_dispersion.cpp; modules/record/statistics/include/SushiProfiler/statistics/dispersion.hpp; modules/record/statistics/source/dispersion.cpp
Small_Dispersion.JitterIsTheMedianSuccessiveStep | keep | 9 | dispersion: jitter is the median successive step | tests/record/statistics/test_dispersion.cpp; modules/record/statistics/include/SushiProfiler/statistics/dispersion.hpp; modules/record/statistics/source/dispersion.cpp
```

### `tests/record/statistics/test_log2_histogram.cpp`

```
Small_Log2Histogram.BinsByPowerOfTwoAndKeepsEmptyBins | keep | 9 | log2_histogram: power-of-two bins, exact at each power, empty bins kept (expected counts derived by hand: 0.5, 1.0, 1.5, 2.0, 3.0, 4.0, 1023, 1024) | tests/record/statistics/test_log2_histogram.cpp; modules/record/statistics/include/SushiProfiler/statistics/log2_histogram.hpp
Small_Log2Histogram.NoPositiveSampleGivesNoBins | keep | 9 | log2_histogram: no positive sample gives no bins | tests/record/statistics/test_log2_histogram.cpp; modules/record/statistics/include/SushiProfiler/statistics/log2_histogram.hpp
```

### `tests/record/statistics/test_sorted_samples.cpp`

```
Small_SortedSamples.SortsAndAnswersTheEnds | keep | 9 | sorted_samples: sorts; min, max and size answer | tests/record/statistics/test_sorted_samples.cpp; modules/record/statistics/include/SushiProfiler/statistics/sorted_samples.hpp
Small_SortedSamples.TheMedianOfAnEvenCountIsTheMidpoint | split | 3 | sorted_samples: median; the name says even count, the first assertion uses an odd count (5 samples, median 3); split into odd and even | tests/record/statistics/test_sorted_samples.cpp; modules/record/statistics/include/SushiProfiler/statistics/sorted_samples.hpp
Small_SortedSamples.PercentilesInterpolateByTheType7Rule | rewrite | 1 | sorted_samples: type-7 percentile; the 90th check uses a bare tolerance literal (1e-12), a forbidden kind; pick data whose interpolation is exact or use the tolerance helper | tests/record/statistics/test_sorted_samples.cpp; modules/record/statistics/include/SushiProfiler/statistics/sorted_samples.hpp
Small_SortedSamples.OneSampleIsEveryPercentile | keep | 9 | sorted_samples: one sample is every percentile | tests/record/statistics/test_sorted_samples.cpp; modules/record/statistics/include/SushiProfiler/statistics/sorted_samples.hpp
```

### `tests/record/statistics/test_steady_state.cpp`

```
Small_SteadyState.CutsTheWarmUpAtTheEarliestBestBatch | keep | 9 | steady_state: MSER-5 cut at the earliest best batch (cut 2 batches = 10 samples; strict < keeps the earliest tie, checked against steady_state.cpp) | tests/record/statistics/test_steady_state.cpp; modules/record/statistics/include/SushiProfiler/statistics/steady_state.hpp; modules/record/statistics/source/steady_state.cpp
Small_SteadyState.LeavesASteadySeriesWhole | keep | 9 | steady_state: steady series cut at 0 | tests/record/statistics/test_steady_state.cpp; modules/record/statistics/include/SushiProfiler/statistics/steady_state.hpp; modules/record/statistics/source/steady_state.cpp
Small_SteadyState.NeedsTwoBatches | keep | 9 | steady_state: fewer than two batches gives 0 (9 samples and empty) | tests/record/statistics/test_steady_state.cpp; modules/record/statistics/include/SushiProfiler/statistics/steady_state.hpp; modules/record/statistics/source/steady_state.cpp
Small_SteadyState.IgnoresATrailingPartialBatch | keep | 9 | steady_state: trailing partial batch ignored (24 samples give 4 batches; the 1000 at index 23 is outside them) | tests/record/statistics/test_steady_state.cpp; modules/record/statistics/include/SushiProfiler/statistics/steady_state.hpp; modules/record/statistics/source/steady_state.cpp
```

### `tests/session/config/test_profiler_config.cpp`

```
Small_ProfilerConfig.ReadsTheDesignedExample | rewrite | 7 | profiler_config: full form; the name names a feature, not a promise, and the body checks a part of the sections; state the promise (every section reads into its field) and check each | tests/session/config/test_profiler_config.cpp; modules/session/config/include/SushiProfiler/config/profiler_config.hpp
Small_ProfilerConfig.AKeyLeftOutKeepsItsDefault | rewrite | 1 | profiler_config: absent key keeps default; the body asserts default constants (forbidden kind, default value); keep one absent-key check | tests/session/config/test_profiler_config.cpp; modules/session/config/include/SushiProfiler/config/profiler_config.hpp
Small_ProfilerConfig.RefusesAMissingOrOtherVersion | keep | 9 | profiler_config: missing or other version refused with its message | tests/session/config/test_profiler_config.cpp; modules/session/config/include/SushiProfiler/config/profiler_config.hpp
Small_ProfilerConfig.RefusesUnknownKeysWithTheirPath | keep | 9 | profiler_config: unknown key refused with its path | tests/session/config/test_profiler_config.cpp; modules/session/config/include/SushiProfiler/config/profiler_config.hpp
Small_ProfilerConfig.RefusesValuesOutOfRange | keep | 9 | profiler_config: out-of-range value refused with its path (four inputs, one promise; rows would be the tidier shape) | tests/session/config/test_profiler_config.cpp; modules/session/config/include/SushiProfiler/config/profiler_config.hpp
Small_ProfilerConfig.RefusesValuesOfTheWrongKind | keep | 9 | profiler_config: wrong-kind value refused with its path | tests/session/config/test_profiler_config.cpp; modules/session/config/include/SushiProfiler/config/profiler_config.hpp
Small_ProfilerConfig.ReadsActiveAll | keep | 9 | profiler_config: active "all" read as active_all, with warm-up kept | tests/session/config/test_profiler_config.cpp; modules/session/config/include/SushiProfiler/config/profiler_config.hpp
Small_ProfilerConfig.AWholeActiveLeavesActiveAllOff | keep | 9 | profiler_config: a whole active leaves active_all off | tests/session/config/test_profiler_config.cpp; modules/session/config/include/SushiProfiler/config/profiler_config.hpp
Small_ProfilerConfig.RefusesAnActiveStringOtherThanAll | keep | 9 | profiler_config: active string other than "all" refused | tests/session/config/test_profiler_config.cpp; modules/session/config/include/SushiProfiler/config/profiler_config.hpp
Small_ProfilerConfig.RefusesAllWithARepeatOfTwo | keep | 9 | profiler_config: active "all" with repeat 2 refused | tests/session/config/test_profiler_config.cpp; modules/session/config/include/SushiProfiler/config/profiler_config.hpp
Small_ProfilerConfig.AcceptsAllWithRepeatOne | merge | 4 | profiler_config: repeat rule with active "all", other input (repeat 1); merge into rows with RefusesAllWithARepeatOfTwo | tests/session/config/test_profiler_config.cpp; modules/session/config/include/SushiProfiler/config/profiler_config.hpp
Small_ProfilerConfig.ReadsKeepPeriodsIncludingZeroAndLeavesItAbsentByDefault | rewrite | 1 | profiler_config: keep_periods zero and 300 read, absent by default; the absent-by-default half asserts a default (forbidden kind); split the default check off | tests/session/config/test_profiler_config.cpp; modules/session/config/include/SushiProfiler/config/profiler_config.hpp
Small_ProfilerConfig.ReadsTheLiveKeysAndKeepsTheirDefaults | rewrite | 1 | profiler_config: live keys read; the defaults half asserts default constants (forbidden kind); keep the set-values check | tests/session/config/test_profiler_config.cpp; modules/session/config/include/SushiProfiler/config/profiler_config.hpp
Small_ProfilerConfig.RefusesALiveKeyOutOfRange | keep | 9 | profiler_config: live key out of range refused | tests/session/config/test_profiler_config.cpp; modules/session/config/include/SushiProfiler/config/profiler_config.hpp
Small_ProfilerConfig.ReadsAWriterEnabledFlagAndHidesItFromTheSettings | keep | 9 | profiler_config: writer enabled flag read out of its settings | tests/session/config/test_profiler_config.cpp; modules/session/config/include/SushiProfiler/config/profiler_config.hpp
Small_ProfilerConfig.RefusesAWriterEnabledFlagThatIsNotABool | keep | 9 | profiler_config: writer enabled that is not a bool refused (checks ok() only; no message, which is acceptable) | tests/session/config/test_profiler_config.cpp; modules/session/config/include/SushiProfiler/config/profiler_config.hpp
Small_ProfilerConfig.LeavesASourcesEnabledKeyInItsSettings | keep | 9 | profiler_config: source keeps its enabled key | tests/session/config/test_profiler_config.cpp; modules/session/config/include/SushiProfiler/config/profiler_config.hpp
Small_ProfilerConfig.RefusesAnUnknownRecordKey | merge | 4 | profiler_config: unknown key refused in the record table; merge into rows with RefusesUnknownKeysWithTheirPath | tests/session/config/test_profiler_config.cpp; modules/session/config/include/SushiProfiler/config/profiler_config.hpp
```

### `tests/session/consumer/test_consumer.cpp`

```
Small_NextPoll.SpeedsUpWhenFullAndSlowsDownWhenQuiet | keep | 9 | next_poll: fast when full, slow when quiet, capped at slowest (checked against consumer.hpp doc: over half full fastest, under a quarter doubles) | tests/session/consumer/test_consumer.cpp; modules/session/consumer/include/SushiProfiler/consumer/consumer.hpp
Medium_Consumer.DrainsWhileRunningAndFinishesOnStop | rewrite | 1 | consumer: drains and stops; the body holds an if inside a loop over trees and nodes and a loop that builds the zones (control flow in a test body, forbidden kind); split the record check into a helper that asserts, and state the promise | tests/session/consumer/test_consumer.cpp; modules/session/consumer/include/SushiProfiler/consumer/consumer.hpp; tests/session/consumer/assembly_rig.hpp
Small_NextPoll.CeilingLimitsTheDoubling | keep | 9 | next_poll: the doubling is capped by the given slowest | tests/session/consumer/test_consumer.cpp; modules/session/consumer/include/SushiProfiler/consumer/consumer.hpp
Small_NextPoll.TheFastestPollStillWinsWhenARingIsFull | keep | 9 | next_poll: fastest wait wins when the ring is full at the cap | tests/session/consumer/test_consumer.cpp; modules/session/consumer/include/SushiProfiler/consumer/consumer.hpp
Medium_Consumer.AWatchedConsumerDrainsAtTheLiveCeiling | rewrite | 1 | consumer: live ceiling; the test waits on the wall clock for up to 5 s with a sleep loop, a forbidden kind (sleep and wall-clock wait); rewrite with an injected clock that the test advances | tests/session/consumer/test_consumer.cpp; modules/session/consumer/include/SushiProfiler/consumer/consumer.hpp
Medium_Consumer.WakeShortensAnIdleSleep | rewrite | 1 | consumer: wake serves an idle consumer; sleeps 2.5 s to synchronise and waits on the wall clock (forbidden kind); rewrite with an injected clock | tests/session/consumer/test_consumer.cpp; modules/session/consumer/include/SushiProfiler/consumer/consumer.hpp
```

### `tests/session/consumer/test_pause_log.cpp`

```
Small_PauseLog.OverlapsAPeriodThatCrossesTheStart | keep | 9 | pause_log: overlap of a period crossing the start | tests/session/consumer/test_pause_log.cpp; modules/session/consumer/include/SushiProfiler/consumer/pause_log.hpp
Small_PauseLog.OverlapsAPeriodThatCrossesTheEnd | merge | 4 | pause_log: overlap at the end and inside; same promise with other inputs; merge into rows with OverlapsAPeriodThatCrossesTheStart | tests/session/consumer/test_pause_log.cpp; modules/session/consumer/include/SushiProfiler/consumer/pause_log.hpp
Small_PauseLog.ABoundaryTouchIsNotAnOverlap | keep | 9 | pause_log: a touch at the boundary is not an overlap (half-open interval) | tests/session/consumer/test_pause_log.cpp; modules/session/consumer/include/SushiProfiler/consumer/pause_log.hpp
Small_PauseLog.AnOpenIntervalOverlapsEverythingAfterItsStart | keep | 9 | pause_log: open interval overlaps everything after its start | tests/session/consumer/test_pause_log.cpp; modules/session/consumer/include/SushiProfiler/consumer/pause_log.hpp
Small_PauseLog.BeginIsIdempotentAndEndWithoutBeginIsIgnored | split | 3 | pause_log: begin is idempotent and end without begin is ignored; two promises in one name and body | tests/session/consumer/test_pause_log.cpp; modules/session/consumer/include/SushiProfiler/consumer/pause_log.hpp
Small_PauseLog.ForgetsClosedIntervalsBeforeATime | keep | 9 | pause_log: forget_before drops closed intervals that ended before the time | tests/session/consumer/test_pause_log.cpp; modules/session/consumer/include/SushiProfiler/consumer/pause_log.hpp
Small_PauseLog.EndsAfterReturnsOnlyLaterEnds | keep | 9 | pause_log: ends_after returns only later ends | tests/session/consumer/test_pause_log.cpp; modules/session/consumer/include/SushiProfiler/consumer/pause_log.hpp
Small_PauseLog.DropsTheOldestIntervalPastTheCap | rewrite | 1 | pause_log: cap drops the oldest; the loop that builds 4101 intervals sits in the test body (control flow, forbidden kind); move the build into a named builder | tests/session/consumer/test_pause_log.cpp; modules/session/consumer/include/SushiProfiler/consumer/pause_log.hpp
Medium_PauseLog.ConcurrentBeginEndAndOverlapsAgree | rewrite | 1 | pause_log: concurrent use agrees; the if in the reader lambda and the loops in the body are control flow in a test body (forbidden kind); move the driver into a named helper and assert the result | tests/session/consumer/test_pause_log.cpp; modules/session/consumer/include/SushiProfiler/consumer/pause_log.hpp
```

### `tests/session/consumer/test_period_assembler.cpp`

```
Small_PeriodAssembler.RecordsARunWithoutATimelineAsOnePeriod | split | 3 | period_assembler: run without timeline is one period; names and sites copied into the record; two promises | tests/session/consumer/test_period_assembler.cpp; tests/session/consumer/assembly_rig.hpp; modules/session/consumer/include/SushiProfiler/consumer/period_assembler.hpp
Small_PeriodAssembler.KeepsTheWindowsPeriodsAndCountsTheRestOutside | split | 3 | period_assembler: window keeps periods with warm-up flags; outside events counted; the name has and | tests/session/consumer/test_period_assembler.cpp; tests/session/consumer/assembly_rig.hpp; modules/session/consumer/include/SushiProfiler/consumer/period_assembler.hpp
Small_PeriodAssembler.LingerKeepsAPeriodOpenAndALaterEventIsLate | split | 3 | period_assembler: linger holds a period open; a late event is counted late | tests/session/consumer/test_period_assembler.cpp; tests/session/consumer/assembly_rig.hpp; modules/session/consumer/include/SushiProfiler/consumer/period_assembler.hpp
Small_PeriodAssembler.APeriodWithoutItsEndMarkIsNotKept | keep | 9 | period_assembler: period without end mark not kept, its events outside | tests/session/consumer/test_period_assembler.cpp; tests/session/consumer/assembly_rig.hpp; modules/session/consumer/include/SushiProfiler/consumer/period_assembler.hpp
Small_PeriodAssembler.AttachesBudgetsAndTags | split | 3 | period_assembler: budget and tag attach to their periods; two event kinds, two promises | tests/session/consumer/test_period_assembler.cpp; tests/session/consumer/assembly_rig.hpp; modules/session/consumer/include/SushiProfiler/consumer/period_assembler.hpp
Small_PeriodAssembler.AsksSourcesForEachPeriodsSpans | rewrite | 1 | period_assembler: StepSource is a test-made stand-in for the owned Source interface (stub kind); rewrite against a module in-memory source | tests/session/consumer/test_period_assembler.cpp; tests/session/consumer/assembly_rig.hpp; modules/session/consumer/include/SushiProfiler/consumer/period_assembler.hpp; modules/session/sources/include/SushiProfiler/sources/source.hpp
Small_PeriodAssembler.GivesSourcesEachPeriodsSteadySpan | rewrite | 1 | period_assembler: PlacingSource records each period it is given (spy and stand-in, stub kind); rewrite against the source's real effect on the record | tests/session/consumer/test_period_assembler.cpp; modules/session/consumer/include/SushiProfiler/consumer/period_assembler.hpp; modules/session/sources/include/SushiProfiler/sources/source.hpp
Small_PeriodAssembler.ASourceInternsANameAndAddsATrackWhileCollecting | rewrite | 1 | period_assembler: PlacingSource stand-in (stub kind); rewrite as above | tests/session/consumer/test_period_assembler.cpp; modules/session/consumer/include/SushiProfiler/consumer/period_assembler.hpp; modules/session/sources/include/SushiProfiler/sources/source.hpp
Small_PeriodAssembler.PlacesWorkInThePeriodHoldingItsStartAndCountsOlderWorkAsDropped | rewrite | 1 | period_assembler: PlacingSource stand-in (stub kind); the name also has and; rewrite as above | tests/session/consumer/test_period_assembler.cpp; modules/session/consumer/include/SushiProfiler/consumer/period_assembler.hpp; modules/session/sources/include/SushiProfiler/sources/source.hpp
Small_PeriodAssembler.KeepsTheZoneRunsOfAnActivePeriodButNotOfAWarmupOne | keep | 9 | period_assembler: zone runs kept for active periods only | tests/session/consumer/test_period_assembler.cpp; tests/session/consumer/assembly_rig.hpp; modules/session/consumer/include/SushiProfiler/consumer/period_assembler.hpp
Small_PeriodAssembler.StreamsPeriodsInOrderAndStopsAFailingWriter | rewrite | 1 | period_assembler: NotingWriter is a spy of the owned StreamWriter (stub kind); rewrite against the real writer stopping on failure | tests/session/consumer/test_period_assembler.cpp; modules/session/consumer/include/SushiProfiler/consumer/period_assembler.hpp; modules/session/stream_writers/include/SushiProfiler/stream_writers/stream_writer.hpp
Small_PeriodAssembler.FoldsLanesAndCountsOpenAndUnmatchedLanes | split | 3 | period_assembler: lanes fold into the tree; open lanes counted; unmatched lanes counted; three promises | tests/session/consumer/test_period_assembler.cpp; tests/session/consumer/assembly_rig.hpp; modules/session/consumer/include/SushiProfiler/consumer/period_assembler.hpp
Small_PeriodAssembler.KeepsThreadsInIndexOrderWithNamesAndDrops | split | 3 | period_assembler: threads in index order; first name kept over a later empty one; drops summed; three promises | tests/session/consumer/test_period_assembler.cpp; tests/session/consumer/assembly_rig.hpp; modules/session/consumer/include/SushiProfiler/consumer/period_assembler.hpp
Small_PeriodAssembler.KeepsTheLastNPeriodsAtStop | keep | 9 | period_assembler: keep_periods trims to the newest N at stop | tests/session/consumer/test_period_assembler.cpp; tests/session/consumer/assembly_rig.hpp; modules/session/consumer/include/SushiProfiler/consumer/period_assembler.hpp
Small_PeriodAssembler.KeepsNoPeriodsWhenZeroButStillStreamsThem | rewrite | 1 | period_assembler: NotingWriter spy (stub kind); rewrite against the real writer | tests/session/consumer/test_period_assembler.cpp; modules/session/consumer/include/SushiProfiler/consumer/period_assembler.hpp; modules/session/stream_writers/include/SushiProfiler/stream_writers/stream_writer.hpp
Small_PeriodAssembler.KeepsEveryPeriodWhenAbsent | keep | 9 | period_assembler: absent keep_periods keeps all | tests/session/consumer/test_period_assembler.cpp; tests/session/consumer/assembly_rig.hpp; modules/session/consumer/include/SushiProfiler/consumer/period_assembler.hpp
Small_PeriodAssembler.ALateEventOfATrimmedPeriodIsStillCountedLate | rewrite | 1 | period_assembler: late event of a trimmed period counted late; the for loop summing threads sits in the body (control flow, forbidden kind); a helper that sums late counts or a single check | tests/session/consumer/test_period_assembler.cpp; modules/session/consumer/include/SushiProfiler/consumer/period_assembler.hpp
Small_PeriodAssembler.ALateEventOlderThanTheTrackedSpansCountsAsOutside | rewrite | 1 | period_assembler: late older than tracked spans counted outside; the body builds 8402 marks in a loop and sums late counts in another loop (control flow, forbidden kind); move the builder into a named helper | tests/session/consumer/test_period_assembler.cpp; modules/session/consumer/include/SushiProfiler/consumer/period_assembler.hpp
Small_PeriodAssembler.TheRecordsTablesGrowWithoutRecopying | rewrite | 2 | period_assembler: the name promises tables grow without recopying, which a test of content cannot see; a mutant that recopies every time survives; no other test covers it; rename to what is checked or assert the growth promise | tests/session/consumer/test_period_assembler.cpp; modules/session/consumer/include/SushiProfiler/consumer/period_assembler.hpp; modules/record/record/include/SushiProfiler/record/run_record.hpp
Small_PeriodAssembler.ASourceFactReachesThePeriodRecord | rewrite | 1 | period_assembler: FactSource is a test-made stand-in for the owned Source interface (stub kind); rewrite against a module in-memory source | tests/session/consumer/test_period_assembler.cpp; modules/session/consumer/include/SushiProfiler/consumer/period_assembler.hpp; modules/session/sources/include/SushiProfiler/sources/source.hpp
Small_PeriodAssembler.MarksAPeriodThatOverlapsAPauseAndNoOther | keep | 9 | period_assembler: a period overlapping a pause carries the paused fact and no other does | tests/session/consumer/test_period_assembler.cpp; tests/session/consumer/assembly_rig.hpp; modules/session/consumer/include/SushiProfiler/consumer/period_assembler.hpp; modules/session/consumer/include/SushiProfiler/consumer/pause_log.hpp
Small_PeriodAssembler.APausedPeriodKeepsItsMarksAndHasNoZones | keep | 9 | period_assembler: paused period keeps marks, has no zones; the next period's zone is kept | tests/session/consumer/test_period_assembler.cpp; tests/session/consumer/assembly_rig.hpp; modules/session/consumer/include/SushiProfiler/consumer/period_assembler.hpp; modules/session/consumer/include/SushiProfiler/consumer/pause_log.hpp
Small_PeriodAssembler.ASourceStillCollectsAPausedPeriod | rewrite | 1 | period_assembler: PlacingSource spy and stand-in (stub kind); rewrite as above | tests/session/consumer/test_period_assembler.cpp; modules/session/consumer/include/SushiProfiler/consumer/period_assembler.hpp; modules/session/sources/include/SushiProfiler/sources/source.hpp
Small_PeriodAssembler.ALaneOpenAcrossAPauseIsResetAfterResume | keep | 9 | period_assembler: lane open across a pause is reset after resume | tests/session/consumer/test_period_assembler.cpp; tests/session/consumer/assembly_rig.hpp; modules/session/consumer/include/SushiProfiler/consumer/period_assembler.hpp; modules/session/consumer/include/SushiProfiler/consumer/pause_log.hpp
Small_PeriodAssembler.OldPauseIntervalsAreForgottenOnceEveryLiveThreadWroteLater | keep | 9 | period_assembler: pause intervals forgotten once live threads wrote later (first row of the forget rule) | tests/session/consumer/test_period_assembler.cpp; tests/session/consumer/assembly_rig.hpp; modules/session/consumer/include/SushiProfiler/consumer/period_assembler.hpp; modules/session/consumer/include/SushiProfiler/consumer/pause_log.hpp
Small_PeriodAssembler.ALiveThreadThatWroteEarlierKeepsAPauseInterval | merge | 4 | period_assembler: forget rule, other input (live thread wrote earlier, interval kept); merge into rows with OldPauseIntervalsAreForgottenOnceEveryLiveThreadWroteLater | tests/session/consumer/test_period_assembler.cpp; modules/session/consumer/include/SushiProfiler/consumer/period_assembler.hpp
Small_PeriodAssembler.ARetiredThreadDoesNotHoldBackForgettingAPause | merge | 4 | period_assembler: forget rule, other input (retired thread); merge into rows with OldPauseIntervalsAreForgottenOnceEveryLiveThreadWroteLater | tests/session/consumer/test_period_assembler.cpp; modules/session/consumer/include/SushiProfiler/consumer/period_assembler.hpp
Small_PeriodAssembler.AWriterEnabledBeforeACloseReceivesThatPeriod | rewrite | 1 | period_assembler: NotingWriter spy (stub kind); rewrite against the real writer | tests/session/consumer/test_period_assembler.cpp; modules/session/consumer/include/SushiProfiler/consumer/stream_writer_set.hpp; modules/session/stream_writers/include/SushiProfiler/stream_writers/stream_writer.hpp
Small_PeriodAssembler.AWriterDisabledBeforeACloseDoesNotReceiveIt | rewrite | 1 | period_assembler: NotingWriter spy (stub kind); rewrite as above | tests/session/consumer/test_period_assembler.cpp; modules/session/consumer/include/SushiProfiler/consumer/stream_writer_set.hpp; modules/session/stream_writers/include/SushiProfiler/stream_writers/stream_writer.hpp
Small_PeriodAssembler.ATogglePostedWhileAPeriodIsOpenTakesEffectAtTheNextClose | rewrite | 1 | period_assembler: NotingWriter spy (stub kind); rewrite as above | tests/session/consumer/test_period_assembler.cpp; modules/session/consumer/include/SushiProfiler/consumer/stream_writer_set.hpp; modules/session/stream_writers/include/SushiProfiler/stream_writers/stream_writer.hpp
Small_PeriodAssembler.PublishesEachClosedPeriodWhenAViewIsWatching | rewrite | 1 | period_assembler: each published period checked in a for loop in the body (control flow, forbidden kind); check the history through a named helper or one period | tests/session/consumer/test_period_assembler.cpp; modules/session/consumer/include/SushiProfiler/consumer/period_assembler.hpp; modules/session/live/include/SushiProfiler/live/closed_period.hpp
Small_PeriodAssembler.PublishesNothingWithoutAView | keep | 9 | period_assembler: nothing published without a view | tests/session/consumer/test_period_assembler.cpp; tests/session/consumer/assembly_rig.hpp; modules/session/consumer/include/SushiProfiler/consumer/period_assembler.hpp
Small_PeriodAssembler.TheClosedPeriodCapsItsRunsButTheRecordKeepsThemAll | keep | 9 | period_assembler: published copy keeps the longest runs up to the cap (closed_period.hpp: the longest win), the record keeps all | tests/session/consumer/test_period_assembler.cpp; tests/session/consumer/assembly_rig.hpp; modules/session/live/include/SushiProfiler/live/closed_period.hpp (read by grep of the RunCap comments)
Small_PeriodAssembler.AClosedPeriodOutlivesTheAssembler | keep | 9 | period_assembler: a closed period is a copy that outlives the assembler and the record | tests/session/consumer/test_period_assembler.cpp; tests/session/consumer/assembly_rig.hpp; modules/session/consumer/include/SushiProfiler/consumer/period_assembler.hpp
Small_PeriodAssembler.PublishesAfterTheWritersAndTheRecord | rewrite | 1 | period_assembler: ProbingWriter is a test-made spy of the owned StreamWriter (stub kind); rewrite against the real writer and the channel | tests/session/consumer/test_period_assembler.cpp; modules/session/consumer/include/SushiProfiler/consumer/period_assembler.hpp; modules/session/stream_writers/include/SushiProfiler/stream_writers/stream_writer.hpp
Small_PeriodAssembler.KeepPeriodsZeroStillPublishesEachPeriod | keep | 9 | period_assembler: keep 0 still publishes each period | tests/session/consumer/test_period_assembler.cpp; tests/session/consumer/assembly_rig.hpp; modules/session/consumer/include/SushiProfiler/consumer/period_assembler.hpp
```

### `tests/session/consumer/test_period_retention.cpp`

```
Small_PeriodRetention.AbsentKeepsEverything | keep | 9 | period_retention: absent keep keeps every period | tests/session/consumer/test_period_retention.cpp; modules/session/consumer/include/SushiProfiler/consumer/period_retention.hpp
Small_PeriodRetention.ZeroClearsAtOnce | keep | 9 | period_retention: keep 0 clears at once | tests/session/consumer/test_period_retention.cpp; modules/session/consumer/include/SushiProfiler/consumer/period_retention.hpp
Small_PeriodRetention.TrimsToNWhenTheListReachesTwoN | keep | 9 | period_retention: running trim waits until 2N then keeps the newest N | tests/session/consumer/test_period_retention.cpp; modules/session/consumer/include/SushiProfiler/consumer/period_retention.hpp
Small_PeriodRetention.FinalTrimLeavesExactlyN | keep | 9 | period_retention: final trim keeps exactly the newest N, in order | tests/session/consumer/test_period_retention.cpp; modules/session/consumer/include/SushiProfiler/consumer/period_retention.hpp
Small_PeriodRetention.KeepsTheNewestPeriodsInOrder | merge | 4 | period_retention: final trim keeps the newest N in order, other input (10 periods, N 4); merge into rows with FinalTrimLeavesExactlyN; the loop in the body is also a sign (rule 7) that the rows would remove | tests/session/consumer/test_period_retention.cpp; modules/session/consumer/include/SushiProfiler/consumer/period_retention.hpp
Small_PeriodRetention.AShorterListThanNIsLeftAlone | keep | 9 | period_retention: list shorter than N left alone | tests/session/consumer/test_period_retention.cpp; modules/session/consumer/include/SushiProfiler/consumer/period_retention.hpp
```

### `tests/session/consumer/test_stream_writer_set.cpp`

```
Small_StreamWriterSet.ADisabledWriterIsNotBegunUntilEnabled | rewrite | 1 | stream_writer_set: LoggingWriter is a spy of the owned StreamWriter interface, made by the test (forbidden kind); rewrite against a module in-memory or real writer | tests/session/consumer/test_stream_writer_set.cpp; modules/session/consumer/include/SushiProfiler/consumer/stream_writer_set.hpp; modules/session/stream_writers/include/SushiProfiler/stream_writers/stream_writer.hpp
Small_StreamWriterSet.ARequestAppliesOnlyInApplyRequests | rewrite | 1 | stream_writer_set: LoggingWriter spy (forbidden kind); rewrite as above | tests/session/consumer/test_stream_writer_set.cpp; modules/session/consumer/include/SushiProfiler/consumer/stream_writer_set.hpp; modules/session/stream_writers/include/SushiProfiler/stream_writers/stream_writer.hpp
Small_StreamWriterSet.TurningOffStopsPeriodsButKeepsTheOutputOpenUntilEnd | rewrite | 1 | stream_writer_set: LoggingWriter spy (forbidden kind); rewrite as above | tests/session/consumer/test_stream_writer_set.cpp; modules/session/consumer/include/SushiProfiler/consumer/stream_writer_set.hpp; modules/session/stream_writers/include/SushiProfiler/stream_writers/stream_writer.hpp
Small_StreamWriterSet.AWriterThatFailsToBeginWhenEnabledIsStoppedAndNamed | rewrite | 1 | stream_writer_set: LoggingWriter spy (forbidden kind); rewrite as above | tests/session/consumer/test_stream_writer_set.cpp; modules/session/consumer/include/SushiProfiler/consumer/stream_writer_set.hpp; modules/session/stream_writers/include/SushiProfiler/stream_writers/stream_writer.hpp
Small_StreamWriterSet.AFailedWriterIgnoresLaterRequests | rewrite | 1 | stream_writer_set: LoggingWriter spy (forbidden kind); rewrite as above | tests/session/consumer/test_stream_writer_set.cpp; modules/session/consumer/include/SushiProfiler/consumer/stream_writer_set.hpp; modules/session/stream_writers/include/SushiProfiler/stream_writers/stream_writer.hpp
Small_StreamWriterSet.EndAllEndsEveryBegunWriterOnceAndNoOther | rewrite | 1 | stream_writer_set: LoggingWriter spy (forbidden kind); rewrite as above | tests/session/consumer/test_stream_writer_set.cpp; modules/session/consumer/include/SushiProfiler/consumer/stream_writer_set.hpp; modules/session/stream_writers/include/SushiProfiler/stream_writers/stream_writer.hpp
Small_StreamWriterSet.AnUnknownNameIsRefused | rewrite | 1 | stream_writer_set: a writer is only registered, so the stub is not needed (forbidden kind); rewrite against a set with no writer or a real one | tests/session/consumer/test_stream_writer_set.cpp; modules/session/consumer/include/SushiProfiler/consumer/stream_writer_set.hpp
Small_StreamWriterSet.BeginEnabledStopsAtTheFirstFailure | rewrite | 1 | stream_writer_set: LoggingWriter spy (forbidden kind); rewrite as above | tests/session/consumer/test_stream_writer_set.cpp; modules/session/consumer/include/SushiProfiler/consumer/stream_writer_set.hpp; modules/session/stream_writers/include/SushiProfiler/stream_writers/stream_writer.hpp
Medium_StreamWriterSet.RequestsFromAnotherThreadNeverTearAPeriod | rewrite | 1 | stream_writer_set: LoggingWriter spy (forbidden kind) and control flow in the body (while, if, for); rewrite with a named driver and the real writer | tests/session/consumer/test_stream_writer_set.cpp; modules/session/consumer/include/SushiProfiler/consumer/stream_writer_set.hpp; modules/session/stream_writers/include/SushiProfiler/stream_writers/stream_writer.hpp
```

### `tests/session/environment/test_environment_contributor.cpp`

```
Small_EnvironmentContributor.StartsFromTheHostAndTheLibraryBuild | resize | 8 | environment_contributor: gather_facts starts from host and build facts; gather_facts calls host_facts, which reads the machine, forbidden in small; resize to medium or inject the host reader; the oracle calls host_facts, a collaborator of the unit, and is accepted | tests/session/environment/test_environment_contributor.cpp; modules/session/environment/include/SushiProfiler/environment/environment_contributor.hpp; modules/session/environment/include/SushiProfiler/environment/record_facts.hpp; modules/session/environment/source/environment_contributor.cpp; modules/session/environment/source/record_facts.cpp
Small_EnvironmentContributor.AppliesContributorsInOrderAndTheLastWins | rewrite | 1 | environment_contributor: LibraryFacts is a test-made implementation of the owned EnvironmentContributor interface, a stand-in source of data (stub kind); it also reads the machine through gather_facts; rewrite against an injected host reader and a module-owned in-memory contributor | tests/session/environment/test_environment_contributor.cpp; modules/session/environment/include/SushiProfiler/environment/environment_contributor.hpp; modules/session/environment/include/SushiProfiler/environment/record_facts.hpp
```

### `tests/session/environment/test_record_facts.cpp`

```
Small_RecordFacts.SetFactAddsThenReplacesInPlace | split | 3 | record_facts: set_fact appends a name and replaces a known one; two actions under test with one check | tests/session/environment/test_record_facts.cpp; modules/session/environment/include/SushiProfiler/environment/record_facts.hpp; modules/session/environment/source/record_facts.cpp
Small_RecordFacts.HostFactsNameTheSystemCPUAndThreads | resize | 8 | record_facts: host_facts names os, cpu, hardware_threads; host_facts reads CPUID and the thread count and the OS, which Small forbids (the environment); resize to medium; the name and order logic should move behind an injected host reader so a small test can see it | tests/session/environment/test_record_facts.cpp; modules/session/environment/include/SushiProfiler/environment/record_facts.hpp; modules/session/environment/source/record_facts.cpp
Small_RecordFacts.BuildFactsNameTheCompilerAndBuildType | keep | 9 | record_facts: library_build_facts names compiler and build_type (compile-time only, small is right) | tests/session/environment/test_record_facts.cpp; modules/session/environment/include/SushiProfiler/environment/record_facts.hpp; modules/session/environment/source/record_facts.cpp
```

### `tests/session/fold/test_lane_tracker.cpp`

```
Small_LaneTracker.GivesEachEventTheInnermostOpenLane | rewrite | 7 | check between actions, one test per promise: the innermost lane, and the lane back to its outer one after a close | tests/session/fold/test_lane_tracker.cpp; tests/session/fold/README.md; modules/session/fold/include/SushiProfiler/fold/lane_tracker.hpp
Small_LaneTracker.AnEndOfALaneNotOpenIsUnmatched | keep | 9 | LaneTracker::lane_of: a lane end with no open lane counts as unmatched | tests/session/fold/test_lane_tracker.cpp; tests/session/fold/README.md; modules/session/fold/include/SushiProfiler/fold/lane_tracker.hpp
Small_LaneTracker.AnInnerLaneClosedFirstLeavesTheOthersOpen | keep | 9 | LaneTracker: closing an inner lane leaves the outer lanes open | tests/session/fold/test_lane_tracker.cpp; tests/session/fold/README.md; modules/session/fold/include/SushiProfiler/fold/lane_tracker.hpp
Small_LaneTracker.ResetClosesEveryLane | keep | 9 | LaneTracker::reset: closes every lane | tests/session/fold/test_lane_tracker.cpp; tests/session/fold/README.md; modules/session/fold/include/SushiProfiler/fold/lane_tracker.hpp
```

### `tests/session/fold/test_period_fold.cpp`

```
Small_PeriodFold.NestsZonesByTimeAndMergesCalls | split | 3 | "nests" and "merges calls" are two promises: nesting by time; calls of one site merge into one node | tests/session/fold/test_period_fold.cpp; tests/session/fold/README.md; modules/session/fold/include/SushiProfiler/fold/period_fold.hpp
Small_PeriodFold.OneSiteUnderTwoParentsIsTwoNodes | keep | 9 | fold_period: one site under two parents is two nodes | tests/session/fold/test_period_fold.cpp; tests/session/fold/README.md; modules/session/fold/include/SushiProfiler/fold/period_fold.hpp
Small_PeriodFold.EqualRunsNestInRingOrder | keep | 9 | fold_period: equal runs nest in ring order | tests/session/fold/test_period_fold.cpp; tests/session/fold/README.md; modules/session/fold/include/SushiProfiler/fold/period_fold.hpp
Small_PeriodFold.SplitsTreesByThreadThenLane | keep | 9 | fold_period: trees split by thread, then lane | tests/session/fold/test_period_fold.cpp; tests/session/fold/README.md; modules/session/fold/include/SushiProfiler/fold/period_fold.hpp
Small_PeriodFold.KeepsEveryRunWithItsDepth | keep | 9 | fold_period: every zone run is kept with its site, begin and depth | tests/session/fold/test_period_fold.cpp; tests/session/fold/README.md; modules/session/fold/include/SushiProfiler/fold/period_fold.hpp
Small_PeriodFold.CountsZonesThatDoNotNest | keep | 9 | fold_period: a zone that does not nest is counted as unmatched per thread | tests/session/fold/test_period_fold.cpp; tests/session/fold/README.md; modules/session/fold/include/SushiProfiler/fold/period_fold.hpp
Small_PeriodFold.FoldsSamplesBySiteThenThread | split | 3 | "folds samples by site then thread" is one promise; "period and lane events are not folded" is a second, checked in the same test | tests/session/fold/test_period_fold.cpp; tests/session/fold/README.md; modules/session/fold/include/SushiProfiler/fold/period_fold.hpp
Small_PeriodFold.TheSameEventsFoldTheSame | keep | 9 | fold_period: the same events fold to the same result | tests/session/fold/test_period_fold.cpp; tests/session/fold/README.md; modules/session/fold/include/SushiProfiler/fold/period_fold.hpp
```

### `tests/session/fold/test_record_clock.cpp`

```
Small_RecordClock.MapsTicksThroughTheCalibration | split | 3 | from_ticks and from_steady are two promises, one per mapping | tests/session/fold/test_record_clock.cpp; tests/session/fold/README.md; modules/session/fold/include/SushiProfiler/fold/record_clock.hpp
Small_RecordClock.TakesSteadyTicksAsTheyAre | keep | 9 | RecordClock with steady ticks: ticks are taken as nanoseconds | tests/session/fold/test_record_clock.cpp; tests/session/fold/README.md; modules/session/fold/include/SushiProfiler/fold/record_clock.hpp
Small_RecordClock.ConvertsOnlyTheFieldsThatHoldTimes | split | 3 | to_record: zone, external zone and counter are three kinds with three field rules | tests/session/fold/test_record_clock.cpp; tests/session/fold/README.md; modules/session/fold/include/SushiProfiler/fold/record_clock.hpp
```

### `tests/session/live/test_closed_period.cpp`

```
Small_ClosedPeriod.KeepsTheLongestRunsAndCountsTheOmitted | keep | 9 | make_closed_period: the longest runs are kept; omitted is the complement of kept, one partition promise | tests/session/live/test_closed_period.cpp; tests/session/live/README.md; modules/session/live/include/SushiProfiler/live/closed_period.hpp
Small_ClosedPeriod.DropsRunsBelowTheMinimumLength | keep | 9 | make_closed_period: runs under min_run_ns go first | tests/session/live/test_closed_period.cpp; tests/session/live/README.md; modules/session/live/include/SushiProfiler/live/closed_period.hpp
Small_ClosedPeriod.KeepsRunsInThreadLaneBeginOrder | rewrite | 1 | forbidden kind: control flow in the test body (a for loop with EXPECT_LT); state the expected sequence as one value instead | tests/session/live/test_closed_period.cpp; tests/session/live/README.md; modules/session/live/include/SushiProfiler/live/closed_period.hpp
Small_ClosedPeriod.ZeroMaxRunsKeepsNone | keep | 9 | make_closed_period: a zero cap keeps no run | tests/session/live/test_closed_period.cpp; tests/session/live/README.md; modules/session/live/include/SushiProfiler/live/closed_period.hpp
Small_ClosedPeriod.AWarmupPeriodHasNoRuns | keep | 9 | make_closed_period: a warm-up period keeps no run | tests/session/live/test_closed_period.cpp; tests/session/live/README.md; modules/session/live/include/SushiProfiler/live/closed_period.hpp
Small_ClosedPeriod.LeavesTheSourcePeriodUntouched | keep | 9 | make_closed_period: the source period is left untouched | tests/session/live/test_closed_period.cpp; tests/session/live/README.md; modules/session/live/include/SushiProfiler/live/closed_period.hpp
Small_ClosedPeriod.CarriesItsRunSequenceOriginAndCounts | keep | 9 | make_closed_period: run, sequence, origin and counts are carried over | tests/session/live/test_closed_period.cpp; tests/session/live/README.md; modules/session/live/include/SushiProfiler/live/closed_period.hpp
Small_ClosedPeriod.CopiesEveryFieldButTheRuns | rewrite | 7 | sign: setup longer than the action and checks together (about twelve lines of fixture); name says "but the runs" while the body compares the whole period under a cap that keeps every run; state the promise as "with a cap that keeps every run, the closed period equals its source" and move the setup into a builder | tests/session/live/test_closed_period.cpp; tests/session/live/README.md; modules/session/live/include/SushiProfiler/live/closed_period.hpp
```

### `tests/session/live/test_live_channel.cpp`

```
Small_LiveChannel.LatestIsNullBeforeAPublication | keep | 9 | LiveChannel: latest is null and history is empty before the first publication | tests/session/live/test_live_channel.cpp; tests/session/live/README.md; modules/session/live/include/SushiProfiler/live/live_channel.hpp
Small_LiveChannel.HistoryReturnsTheLastNOldestFirst | split | 3 | history returns the last N oldest first; latest and published are other promises checked in the same test | tests/session/live/test_live_channel.cpp; tests/session/live/README.md; modules/session/live/include/SushiProfiler/live/live_channel.hpp
Small_LiveChannel.TheRingOverwritesItsOldestWhenFull | keep | 9 | LiveChannel: the ring overwrites its oldest period when full | tests/session/live/test_live_channel.cpp; tests/session/live/README.md; modules/session/live/include/SushiProfiler/live/live_channel.hpp
Small_LiveChannel.AReaderKeepsAnOverwrittenPeriodAlive | rewrite | 1 | forbidden kind: control flow in the test body (a range-for with EXPECT); the held-alive promise and the freed promise are two checks to state separately | tests/session/live/test_live_channel.cpp; tests/session/live/README.md; modules/session/live/include/SushiProfiler/live/live_channel.hpp
Small_LiveChannel.WatchedFollowsAttachAndDetach | keep | 9 | LiveChannel::attach and detach: watched while a viewer is counted | tests/session/live/test_live_channel.cpp; tests/session/live/README.md; modules/session/live/include/SushiProfiler/live/live_channel.hpp
Small_LiveChannel.BeginRunClearsTheHistoryAndBumpsTheRun | split | 3 | begin_run empties the history and latest; begin_run numbers the run; a held period survives the new run | tests/session/live/test_live_channel.cpp; tests/session/live/README.md; modules/session/live/include/SushiProfiler/live/live_channel.hpp
Medium_LiveChannel.ReadersNeverSeeSequenceGoBackwards | rewrite | 1 | forbidden kind: control flow in the test body (while, for and if inside the reader lambda, which is not a row table or a builder); move the reader loop into a named helper, keep the promise | tests/session/live/test_live_channel.cpp; tests/session/live/README.md; modules/session/live/include/SushiProfiler/live/live_channel.hpp
```

### `tests/session/live/test_live_view.cpp`

```
Small_LiveView.ADefaultViewIsInvalidAndReadsNothing | keep | 9 | LiveView: a default view is invalid and reads nothing | tests/session/live/test_live_view.cpp; tests/session/live/README.md; modules/session/live/include/SushiProfiler/live/live_view.hpp
Small_LiveView.WatchedFollowsCopiesAndMoves | keep | 9 | LiveView: copies and moves keep one viewer each | tests/session/live/test_live_view.cpp; tests/session/live/README.md; modules/session/live/include/SushiProfiler/live/live_view.hpp
Small_LiveView.ACopyAssignmentMovesTheViewerCount | keep | 9 | LiveView: copy assignment moves the count | tests/session/live/test_live_view.cpp; tests/session/live/README.md; modules/session/live/include/SushiProfiler/live/live_view.hpp
Small_LiveView.AViewOutlivesItsChannelsOwner | keep | 9 | LiveView: reads its channel after the owner releases it | tests/session/live/test_live_view.cpp; tests/session/live/README.md; modules/session/live/include/SushiProfiler/live/live_view.hpp
```

### `tests/session/live/test_name_snapshot.cpp`

```
Small_NameSnapshotCache.ReusesTheSnapshotWhileTablesAreUnchanged | split | 3 | cache reuse is one promise; id resolution (name, site, track, thread name) is a second, checked in the same test | tests/session/live/test_name_snapshot.cpp; tests/session/live/README.md; modules/session/live/include/SushiProfiler/live/name_snapshot.hpp
Small_NameSnapshotCache.RebuildsWhenANameASiteATrackOrAThreadNameGrew | rewrite | 7 | sign: check between four actions; the same promise over four kinds of change is rows, one row per table | tests/session/live/test_name_snapshot.cpp; tests/session/live/README.md; modules/session/live/include/SushiProfiler/live/name_snapshot.hpp
Small_NameSnapshotCache.ASnapshotKeepsItsTextAfterTheRecordGrows | keep | 9 | NameSnapshot: keeps its text after the record grows | tests/session/live/test_name_snapshot.cpp; tests/session/live/README.md; modules/session/live/include/SushiProfiler/live/name_snapshot.hpp
Small_NameSnapshotCache.UnknownIdsResolveToNothing | keep | 9 | NameSnapshot: an unknown id resolves to nothing | tests/session/live/test_name_snapshot.cpp; tests/session/live/README.md; modules/session/live/include/SushiProfiler/live/name_snapshot.hpp
```

### `tests/session/live/test_period_publisher.cpp`

```
Small_PeriodPublisher.PublishesNothingWhileTheChannelIsNotWatched | keep | 9 | PeriodPublisher: publishes nothing while the channel is unwatched | tests/session/live/test_period_publisher.cpp; tests/session/live/README.md; modules/session/live/include/SushiProfiler/live/period_publisher.hpp
Small_PeriodPublisher.PublishesWithConsecutiveSequencesAndTheChannelsRun | keep | 9 | PeriodPublisher: consecutive sequences and the channel's run and origin; two publications are the minimum that show consecutive numbering | tests/session/live/test_period_publisher.cpp; tests/session/live/README.md; modules/session/live/include/SushiProfiler/live/period_publisher.hpp
Small_PeriodPublisher.SharesNamesUntilATableGrows | keep | 9 | PeriodPublisher: periods share a name snapshot until a table grows | tests/session/live/test_period_publisher.cpp; tests/session/live/README.md; modules/session/live/include/SushiProfiler/live/period_publisher.hpp
Small_PeriodPublisher.SumsTheRecordsLateAndDroppedCounts | split | 3 | the thread late and dropped counts are summed; the record's spans-dropped and outside counts are carried over, two promises | tests/session/live/test_period_publisher.cpp; tests/session/live/README.md; modules/session/live/include/SushiProfiler/live/period_publisher.hpp
```

### `tests/session/registry/test_registry.cpp`

```
Small_Registry.MakesASourceByNameAndRefusesAnUnknownOne | split | 3 | "makes by name" and "refuses an unknown name" are two results that can break apart | tests/session/registry/test_registry.cpp; tests/session/registry/README.md; modules/session/registry/include/SushiProfiler/registry/registry.hpp
Small_Registry.ALibraryRegistersItsSourceAndContributorThroughItsParts | split | 3 | registration through parts; source placing work in periods and counting late work; contributor fact reaching the gathered facts | tests/session/registry/test_registry.cpp; tests/session/registry/README.md; modules/session/registry/include/SushiProfiler/registry/registry.hpp
Small_Registry.ANameIsTaken | keep | 9 | Registry::add_source and add_stream_writer and add_record_writer: a name is taken once across kinds | tests/session/registry/test_registry.cpp; tests/session/registry/README.md; modules/session/registry/include/SushiProfiler/registry/registry.hpp
Small_Registry.KnowsWhichWritersStreamAndListsEveryWriter | split | 3 | Registry::streams and the unknown-writer listing are separate functions and promises | tests/session/registry/test_registry.cpp; tests/session/registry/README.md; modules/session/registry/include/SushiProfiler/registry/registry.hpp
Small_Registry.ReadsTheBuiltInWritersSettings | split | 3 | terminal top accepted; unknown key refused; record path required; chrome_trace path accepted; four promises | tests/session/registry/test_registry.cpp; tests/session/registry/README.md; modules/session/registry/include/SushiProfiler/registry/registry.hpp
Small_Registry.KeepsContributorsInOrder | keep | 9 | Registry::add_contributor: contributors are kept in the order added | tests/session/registry/test_registry.cpp; tests/session/registry/README.md; modules/session/registry/include/SushiProfiler/registry/registry.hpp
```

### `tests/session/session/test_session.cpp`

```
Medium_Session.RecordsTenPeriodsFromTwoThreads | rewrite | 1 | forbidden kind: control flow in the test body (for loops with EXPECT_EQ and a range-for over tables); it also states six promises, to be split when rewritten | tests/session/session/test_session.cpp; tests/session/session/README.md; modules/session/session/include/SushiProfiler/session/session.hpp
Small_Session.RefusesAnUnknownAnalysisOrWriter | rewrite | 1 | forbidden kind: a small test reaches the process-wide session (Session::start takes the one running slot, session.cpp:266); the analysis refusal is tested nowhere else; move to medium, or split the name check from start behind a seam first | tests/session/session/test_session.cpp; tests/session/session/README.md; modules/session/session/include/SushiProfiler/session/session.hpp; modules/session/session/source/session.cpp (grep)
Small_Session.RunsOneSessionPerProcess | rewrite | 1 | forbidden kind: a small test reaches the process-wide session slot; the one-session promise is only stated here; move to medium | tests/session/session/test_session.cpp; tests/session/session/README.md; modules/session/session/include/SushiProfiler/session/session.hpp; modules/session/session/source/session.cpp (grep)
Small_Session.ADisabledSessionStatesItsFactsAndRecordsNothing | rewrite | 1 | forbidden kind: a small test reaches the process-wide session slot; the name joins two promises with "and" | tests/session/session/test_session.cpp; tests/session/session/README.md; modules/session/session/include/SushiProfiler/session/session.hpp; modules/session/session/source/session.cpp (grep)
Medium_Session.LiveViewShowsEachClosedPeriodWhileTheSessionRuns | rewrite | 1 | forbidden kind: a sleep to synchronise (wait_until polls with sleep_for) and control flow in the test body (a loop with EXPECT_EQ) | tests/session/session/test_session.cpp; tests/session/session/README.md; modules/session/session/include/SushiProfiler/session/session.hpp
Medium_Session.ALiveViewOutlivesTheSessionAndKeepsItsLastPeriods | rewrite | 1 | forbidden kind: a sleep to synchronise (wait_until); the outlive promise is stated at unit level only, so the session-level check must stay, waiting on the clock instead | tests/session/session/test_session.cpp; tests/session/session/README.md; modules/session/session/include/SushiProfiler/session/session.hpp
Medium_Session.AViewTakenBeforeStopKeepsWorkingAfterRestart | rewrite | 1 | forbidden kind: a sleep to synchronise (wait_until) and control flow in the test body | tests/session/session/test_session.cpp; tests/session/session/README.md; modules/session/session/include/SushiProfiler/session/session.hpp
Medium_Session.KeepsTheLastNPeriodsInTheStoppedRecord | resize | 8 | keep_periods trimming is a decision on values, seen small once the trimming is a function over the period list; the session's threads and clock make it medium today; build that seam first | tests/session/session/test_session.cpp; tests/session/session/README.md; modules/session/session/include/SushiProfiler/session/session.hpp
Medium_Session.KeepPeriodsZeroReturnsNoPeriodsButLiveStillPublishes | split | 3 | "but" joins two results: the record holds no periods; the live view still publishes | tests/session/session/test_session.cpp; tests/session/session/README.md; modules/session/session/include/SushiProfiler/session/session.hpp
Medium_Session.AWriterStartedDisabledWritesNothingUntilEnabledAtABoundary | rewrite | 1 | forbidden kind: a sleep to synchronise (wait_until); the test also leaves its trace file in the temporary folder | tests/session/session/test_session.cpp; tests/session/session/README.md; modules/session/session/include/SushiProfiler/session/session.hpp
Medium_Session.RefusesAWriterToggleForAnUnknownOrRecordWriterName | split | 3 | refusal of an unknown name, refusal of a record writer, Queued for a stream writer and NotRecording after stop are four promises | tests/session/session/test_session.cpp; tests/session/session/README.md; modules/session/session/include/SushiProfiler/session/session.hpp
Medium_SessionPause.PauseAndResumeReportWhetherTheyTookEffect | resize | 8 | the pause state transitions are decisions on values, seen small once the state is a value type; the session's mutex and process-wide slot make it medium today | tests/session/session/test_session.cpp; tests/session/session/README.md; modules/session/session/include/SushiProfiler/session/session.hpp
Medium_SessionPause.PauseMarksTheCrossedPeriodsAndResumeRecordsAgain | rewrite | 1 | forbidden kind: a sleep to make periods have length (breathe calls sleep_for); the name joins two promises with "and" | tests/session/session/test_session.cpp; tests/session/session/README.md; modules/session/session/include/SushiProfiler/session/session.hpp
Medium_SessionPause.SourcesAreNotWantedWhilePaused | rewrite | 1 | forbidden kind: a sleep to synchronise (wait_until); the flag is process-wide (Collector::sources_wanted) | tests/session/session/test_session.cpp; tests/session/session/README.md; modules/session/session/include/SushiProfiler/session/session.hpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp (grep)
Medium_Session.ASourceDelayHoldsPeriodsOpenAndLosesNone | rewrite | 1 | forbidden kind: a sleep in the marking loop (sleep_for of 10 ms); the name joins two promises with "and" | tests/session/session/test_session.cpp; tests/session/session/README.md; modules/session/session/include/SushiProfiler/session/session.hpp
Small_Session.RefusesASourceDelayAboveTheLimit | rewrite | 1 | forbidden kind: a small test reaches the process-wide session slot and Collector::sources_wanted; the 60 000 ms refusal is tested nowhere else; move to medium | tests/session/session/test_session.cpp; tests/session/session/README.md; modules/session/session/include/SushiProfiler/session/session.hpp; modules/session/session/source/session.cpp (grep); modules/capture/collector/include/SushiProfiler/collector/collector.hpp (grep)
Medium_Session.EveryPeriodWindowRecordsAllAfterWarmup | rewrite | 1 | forbidden kind: control flow in the test body (a for loop with EXPECT_EQ over 203 periods); the window unit test covers phases only, so the session-level promise stays | tests/session/session/test_session.cpp; tests/session/session/README.md; modules/session/session/include/SushiProfiler/session/session.hpp
Medium_Session.ARestartAfterStopReadsASecondRecord | rewrite | 1 | forbidden kind: control flow in the test body (a loop with EXPECT_EQ and zone_sites); the name covers the second record and the refused restart, several promises | tests/session/session/test_session.cpp; tests/session/session/README.md; modules/session/session/include/SushiProfiler/session/session.hpp
Medium_Session.ASourceUsesItsBeginContextInCollect | rewrite | 1 | forbidden kind: a sleep (breathe) and control flow in the test body (a loop with ASSERT and EXPECT) | tests/session/session/test_session.cpp; tests/session/session/README.md; modules/session/session/include/SushiProfiler/session/session.hpp
Medium_Session.ASourceFactReachesTheStoppedRecordAndItsFile | rewrite | 1 | forbidden kind: control flow in the test body (a loop with ASSERT and EXPECT); the name joins two promises with "and"; the record file is left in the temporary folder | tests/session/session/test_session.cpp; tests/session/session/README.md; modules/session/session/include/SushiProfiler/session/session.hpp
Medium_Session.ControlCallsAreSerialisedAgainstStopAndRestart | rewrite | 1 | forbidden kind: control flow in the test body (if and while in the controller thread, if and a counted loop in the test body) | tests/session/session/test_session.cpp; tests/session/session/README.md; modules/session/session/include/SushiProfiler/session/session.hpp
Small_Session.RecordsKeepPeriodsInItsFacts | rewrite | 1 | forbidden kind: a small test reaches the process-wide session slot (Session::start); control flow in the test body (a range-for with ASSERT) | tests/session/session/test_session.cpp; tests/session/session/README.md; modules/session/session/include/SushiProfiler/session/session.hpp; modules/session/session/source/session.cpp (grep)
```

### `tests/session/sources/test_source.cpp`

```
Small_Source.BeginsCollectsEachPeriodAndEnds | split | 3 | begin adds a track and a name; collect places a span at its record time; end runs; three promises | tests/session/sources/test_source.cpp; tests/session/sources/README.md; modules/session/sources/include/SushiProfiler/sources/source.hpp
Small_Source.ASourceThatCannotStartSaysWhy | keep | 9 | Source::begin: a failed begin states its reason and adds no track | tests/session/sources/test_source.cpp; tests/session/sources/README.md; modules/session/sources/include/SushiProfiler/sources/source.hpp
Small_Source.ASourceStatesNoDelayByDefault | rewrite | 1 | forbidden kind: a default value; the default is a documented promise (sources/README.md) that no other test checks; rewrite it as a session-level check that a source which states no delay holds no period open | tests/session/sources/test_source.cpp; tests/session/sources/README.md; modules/session/sources/include/SushiProfiler/sources/source.hpp
Small_Source.AnOverridingSourceStatesItsDelay | delete | 1 | forbidden kind: a constant returned by a class the test owns; the delay promise is tested through the session with ProbeSource and the 61 000 ms refusal in tests/session/session/test_session.cpp | tests/session/sources/test_source.cpp; tests/session/sources/README.md; modules/session/sources/include/SushiProfiler/sources/source.hpp; tests/session/session/test_session.cpp
```

### `tests/session/sources/test_source_context.cpp`

```
Small_SourceContext.InternsIntoTheSessionNames | keep | 9 | SourceContext::intern: the same text gives the same id | tests/session/sources/test_source_context.cpp; tests/session/sources/README.md; modules/session/sources/include/SushiProfiler/sources/source_context.hpp
Small_SourceContext.AddsATrackOnceAndKeepsItsID | keep | 9 | SourceContext::add_track: a track is added once and keeps its id | tests/session/sources/test_source_context.cpp; tests/session/sources/README.md; modules/session/sources/include/SushiProfiler/sources/source_context.hpp
Small_SourceContext.CountsTimeFromTheOrigin | keep | 9 | SourceContext::to_record_ns: counts time from the origin and clamps earlier times to 0 | tests/session/sources/test_source_context.cpp; tests/session/sources/README.md; modules/session/sources/include/SushiProfiler/sources/source_context.hpp
```

### `tests/session/sources/test_span_sink.cpp`

```
Small_SpanSink.StoresASpanAndItsAttributes | split | 3 | "stores a span" and "stores its attributes" can break apart; checked in one test | tests/session/sources/test_span_sink.cpp; tests/session/sources/README.md; modules/session/sources/include/SushiProfiler/sources/span_sink.hpp
Small_SpanSink.RefusesSpansTheRecordCannotHold | rewrite | 7 | sign: several calls with a check after each, one promise over four inputs; rows, one per refusal reason | tests/session/sources/test_span_sink.cpp; tests/session/sources/README.md; modules/session/sources/include/SushiProfiler/sources/span_sink.hpp
Small_SpanSink.CountsLateSpansASourceDroppedApartFromRefusedOnes | keep | 9 | SpanSink::drop_late: counts late spans apart from refused ones | tests/session/sources/test_span_sink.cpp; tests/session/sources/README.md; modules/session/sources/include/SushiProfiler/sources/span_sink.hpp
Small_SpanSink.RefusesAttributesOnABadRowKeyValueOrKind | rewrite | 7 | sign: several calls with a check after each, one promise over five inputs; rows, one per bad input | tests/session/sources/test_span_sink.cpp; tests/session/sources/README.md; modules/session/sources/include/SushiProfiler/sources/span_sink.hpp
Small_SpanSink.SetsAFactOnThePeriod | keep | 9 | SpanSink::set_fact: sets a fact; two facts show that facts accumulate, one promise | tests/session/sources/test_span_sink.cpp; tests/session/sources/README.md; modules/session/sources/include/SushiProfiler/sources/span_sink.hpp
Small_SpanSink.ReplacesAFactOfTheSameName | keep | 9 | SpanSink::set_fact: replaces a fact of the same name | tests/session/sources/test_span_sink.cpp; tests/session/sources/README.md; modules/session/sources/include/SushiProfiler/sources/span_sink.hpp
Small_SpanSink.RefusesAnEmptyFactName | keep | 9 | SpanSink::set_fact: refuses an empty name | tests/session/sources/test_span_sink.cpp; tests/session/sources/README.md; modules/session/sources/include/SushiProfiler/sources/span_sink.hpp
Small_SpanSink.RefusesAFactOnASinkBuiltWithoutFacts | keep | 9 | SpanSink::set_fact: refuses every call on a sink built without facts | tests/session/sources/test_span_sink.cpp; tests/session/sources/README.md; modules/session/sources/include/SushiProfiler/sources/span_sink.hpp
```

### `tests/session/stream_writers/test_chrome_trace_writer.cpp`

```
Medium_ChromeTraceWriter.WritesZonesOnThreadAndLaneRows | resize | 8 | the row mapping is logic behind the file boundary and reaches a small test only through a seam that builds the events apart from the file write; the test also leaves its trace file in the temporary folder | tests/session/stream_writers/test_chrome_trace_writer.cpp; tests/session/stream_writers/README.md; modules/session/stream_writers/include/SushiProfiler/stream_writers/chrome_trace_writer.hpp
Medium_ChromeTraceWriter.WritesThePeriodOnItsTimelineRow | resize | 8 | the timeline row mapping is logic behind the file boundary; seam first, as for WritesZonesOnThreadAndLaneRows | tests/session/stream_writers/test_chrome_trace_writer.cpp; tests/session/stream_writers/README.md; modules/session/stream_writers/include/SushiProfiler/stream_writers/chrome_trace_writer.hpp
Medium_ChromeTraceWriter.WritesCountersInstantsAndFlows | split | 3 | counter, instant and flow are three promises with three event shapes | tests/session/stream_writers/test_chrome_trace_writer.cpp; tests/session/stream_writers/README.md; modules/session/stream_writers/include/SushiProfiler/stream_writers/chrome_trace_writer.hpp
Medium_ChromeTraceWriter.WritesSpansOnDeviceTrackRows | split | 3 | device-track row placement; queue time and attributes as arguments, two promises | tests/session/stream_writers/test_chrome_trace_writer.cpp; tests/session/stream_writers/README.md; modules/session/stream_writers/include/SushiProfiler/stream_writers/chrome_trace_writer.hpp
Medium_ChromeTraceWriter.AnUnwritablePathIsReported | keep | 9 | ChromeTraceWriter: an unwritable path is reported and every later call fails | tests/session/stream_writers/test_chrome_trace_writer.cpp; tests/session/stream_writers/README.md; modules/session/stream_writers/include/SushiProfiler/stream_writers/chrome_trace_writer.hpp
```

### `tests/session/stream_writers/test_stream_writer.cpp`

```
Small_StreamWriter.ReceivesBeginEachPeriodAndEnd | delete | 2 | the test subclasses the interface and calls its own methods; no repository code runs, so no mutant can fail it; the call-order promise is the driver's and is checked by tests/session/consumer/test_stream_writer_set.cpp (grep) | tests/session/stream_writers/test_stream_writer.cpp; tests/session/stream_writers/README.md; modules/session/stream_writers/include/SushiProfiler/stream_writers/stream_writer.hpp; tests/session/consumer/test_stream_writer_set.cpp (grep)
```

### `tests/session/window/test_window.cpp`

```
Small_WindowPhases.PlacesEachPeriodInItsCycle | rewrite | 1 | forbidden kind: control flow in the test body (a for loop with EXPECT_EQ over the expected phases); state each index and phase as a row | tests/session/window/test_window.cpp; tests/session/window/README.md; modules/session/window/include/SushiProfiler/window/window.hpp
Small_WindowPhases.RepeatZeroNeverEnds | keep | 9 | Window::phase_of: repeat 0 never ends | tests/session/window/test_window.cpp; tests/session/window/README.md; modules/session/window/include/SushiProfiler/window/window.hpp
Small_Window.WritesTheRecordedMarksAndTheOneAfter | rewrite | 1 | forbidden kind: a small test reaches the process-wide collector (Collector::instance()); the gate is the promise, so move to medium, or hand the window a collector seam | tests/session/window/test_window.cpp; tests/session/window/README.md; modules/session/window/include/SushiProfiler/window/window.hpp; modules/capture/collector/include/SushiProfiler/collector/collector.hpp (grep)
Small_WindowPhases.ActiveAllRecordsEveryPeriodAfterWarmup | rewrite | 1 | forbidden kind: control flow in the test body (a counted loop with EXPECT_EQ over periods); state the promise over named sample indices as rows | tests/session/window/test_window.cpp; tests/session/window/README.md; modules/session/window/include/SushiProfiler/window/window.hpp
Small_WindowPhases.ActiveAllAppliesSkipWaitAndWarmupOnce | rewrite | 7 | sign: assertions about different results of seven phase_of calls; one row per period index | tests/session/window/test_window.cpp; tests/session/window/README.md; modules/session/window/include/SushiProfiler/window/window.hpp
Small_WindowPhases.ActiveAllRecordsFromTheFirstPeriodWithNoWarmup | keep | 9 | Window with active_all and no warm-up: records from the first period | tests/session/window/test_window.cpp; tests/session/window/README.md; modules/session/window/include/SushiProfiler/window/window.hpp
```

### `tests/tools/record_tool/test_record_tool.cpp`

```
Medium_RecordTool.ValidatesARecordAndRefusesABadOrMissingOne | split | 3 | accepting a valid record and refusing a bad one are separate results; the missing-file exit code is a third | tests/tools/record_tool/test_record_tool.cpp; tests/tools/record_tool/README.md; modules/tools/record_tool/include/SushiProfiler/record_tool/record_tool.hpp; modules/tools/record_tool/source/tool_arguments.cpp
Medium_RecordTool.ShowsTheNamedAnalysesOrRefusesAnUnknownOne | split | 3 | show prints only the named analyses; the refusals of an unknown analysis and a bad top are separate | tests/tools/record_tool/test_record_tool.cpp; tests/tools/record_tool/README.md; modules/tools/record_tool/include/SushiProfiler/record_tool/record_tool.hpp; modules/tools/record_tool/source/tool_arguments.cpp
Medium_RecordTool.ComparesTwoRecordsAndRefusesDifferentBuilds | split | 3 | compare names the faster record; refusal of differing builds; the allowed mismatch are three results | tests/tools/record_tool/test_record_tool.cpp; tests/tools/record_tool/README.md; modules/tools/record_tool/include/SushiProfiler/record_tool/record_tool.hpp; modules/tools/record_tool/source/tool_arguments.cpp
Medium_RecordTool.ChecksARecordAndFailsWhenACheckFails | split | 3 | passing output; failing output; refusal of an unparsable check file are separate results | tests/tools/record_tool/test_record_tool.cpp; tests/tools/record_tool/README.md; modules/tools/record_tool/include/SushiProfiler/record_tool/record_tool.hpp; modules/tools/record_tool/source/tool_arguments.cpp
Small_RecordTool.ExplainsItsUsage | split | 3 | usage text, help, unknown command, unknown option and missing value are five promises; the option errors come before any file is read, so the size is right | tests/tools/record_tool/test_record_tool.cpp; tests/tools/record_tool/README.md; modules/tools/record_tool/source/tool_arguments.cpp
```

### `tests/tools/umbrella/test_SushiProfiler.cpp`

```
Small_SushiProfiler.UmbrellaNamesTheVersion | rewrite | 1 | forbidden kind: a trivial constant (a version string); the version is checked exactly in tests/foundation/version/test_version.cpp:41, so the umbrella's own promise (names reachable through the umbrella alone) is the one to test, not the constant | tests/tools/umbrella/test_SushiProfiler.cpp; tests/tools/umbrella/README.md; modules/tools/umbrella/include/SushiProfiler/SushiProfiler.hpp; tests/foundation/version/test_version.cpp
```

### `tools/tests/test_check_test_additions.py`

```
CheckTestAdditionsTest.test_a_commit_at_its_limit_passes_and_one_over_it_is_named | delete | 3 | tools/testing/check_test_additions.py was removed from the shared tools upstream (sushiskills tools/testing holds no such file); the copy in this repository and its test are a leftover | tools/tests/test_check_test_additions.py
CheckTestAdditionsTest.test_a_fix_has_the_smaller_limit | delete | 3 | the same removal from the shared tools; leftover copy | tools/tests/test_check_test_additions.py
CheckTestAdditionsTest.test_a_second_program_needs_the_owners_line | delete | 3 | the same removal from the shared tools; leftover copy | tools/tests/test_check_test_additions.py
CheckTestAdditionsTest.test_the_owners_line_approves_a_second_program | delete | 3 | the same removal from the shared tools; leftover copy | tools/tests/test_check_test_additions.py
CheckTestAdditionsTest.test_a_repository_that_sets_no_base_commit_is_not_checked | delete | 3 | the same removal from the shared tools; leftover copy | tools/tests/test_check_test_additions.py
```

### `tools/tests/test_test_budget_settings.py`

```
TestBudgetSettingsTest.test_the_skill_table_matches_the_settings | rewrite | 1 | forbidden kind: control flow in the body (the skipTest guard); it is the only test of the promise that the skill's Budget table equals K_TEST_BUDGET; this repository has no skills/ folder, so the test always skips and checks nothing here; rewrite with the skip moved into the fixture | tools/tests/test_test_budget_settings.py; tools/settings.py
TestBudgetSettingsTest.test_reads_a_number_with_a_thousands_space_and_a_trailing_note | split | 3 | several promises in one body: a thousands-spaced number parses, a trailing note is dropped, a row no key names is ignored, rows outside the Budget heading are ignored; read_budget is defined in this test file, not in a module, so it needs a home before the split tests can import it | tools/tests/test_test_budget_settings.py
TestBudgetSettingsTest.test_names_each_key_that_differs | resize | 8 | differences() names each differing key in sorted order, None for a missing side; plain values, so small; the file has no size label, which fails collection; resize to small, no seam needed | tools/tests/test_test_budget_settings.py; tools/settings.py
TestBudgetSettingsTest.test_every_row_of_the_table_has_a_constant | resize | 8 | the label map and K_TEST_BUDGET name the same keys; a comparison of values, so small; Q4: the skill-table test checks the same key set at a larger size, so this smaller copy stays; no size label, resize to small | tools/tests/test_test_budget_settings.py; tools/settings.py
```

## Adds

Promises a header or a port states that no test checks: `unit: promise | oracle | size`.

```
collector: thread_index returns K_NO_THREAD for a thread without a ring | specified value | medium
collector: ring returns null for a thread after it retires | contract | medium
thread_ring: fill is 1.0 when the ring is full | boundary | small
thread_ring: set_name with empty text reads empty | specified value | small
dynamic_site_table: an empty unit leaves the site's unit null | specified value | small
period_timeline: a tag while the collector is closed writes nothing | contract | medium
site_registry: info of id 0 or of an id never given is empty | contract | small
cli: sp test --all passes every_size to the service | contract | small
cli: sp backlog and sp design commands are registered on the app | contract | medium
project: a clean build removes the tree before configuring | contract | medium
calibration: the calibration extrapolates past the second point along the same line | specified value | small
json_value: equality compares nested arrays and objects member by member | expected truth | small
json_field_reader: below refuses an id at or past the table count, naming the table | expected refusal | small
json_field_reader: all_below refuses a column holding an id past the table, naming the column | expected refusal | small
json_field_reader: length refuses a column of the wrong entry count, naming the key | expected refusal | small
json_field_reader: integers refuses a value outside its range, naming its path | expected refusal | small
records: record_zone_at keeps a dynamic name on the event | drained event | small
breakdown: rows are per timeline, each with its own groups and uncovered row | contract | small
period_breakdown: groups of equal self time are ordered by name | contract | small
checks: at_least_ns on a time subject is read as its lower bound | contract | small
verdict: a check reads active periods only, not warm-up | contract | small
verdict: p50 gives the same value as median | metamorphic relation | small
period_record: equality compares index, timeline, thread, warmup, budget, facts and samples | contract | small
run_record: equality compares environment, build, tags, names, tracks, threads and periods | contract | small
span_table: add_column accepts a column read from a file, with one entry per span | contract | small
dispersion: coefficient of variation of a negative mean is positive | specified value | small
pause_log: forget_before keeps an open interval | specified value | small
stream_writer_set: requests for one writer apply in order and the last one wins | specified value | small
NameSnapshot::sites: returns every site indexed by SiteID, for break_down_period() | specified value | small
NameSnapshot::matches: true only for the record whose tables and thread names the snapshot copied | specified value | small
SpanSink::set_real: stores a real attribute on a real key; the accepted path is untested | specified value | small
: none. The rules of check_test_additions are a leftover and are not in scope; the settings promise is tested by tools/tests/test_test_budget_settings.py, and its skill-side half cannot run in this repository. Headers and modules read for this section: tools/tests/test_check_test_additions.py; tools/tests/test_test_budget_settings.py; tools/settings.py.
```
