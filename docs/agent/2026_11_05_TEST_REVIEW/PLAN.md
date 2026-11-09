# Test review: plan

**Status:** Open — under way since 2026-11-09, one worker, under the owner's rules of 2026-11-07

Applies the verdicts of `REPORT.md`, in hierarchy order: the shared helpers, adapters and seams the
rewrites need come first, then the defects the review found, then each module's tests. A module job
applies every record of its module: rewrite, resize, split, merge and delete as the record says, and
writes each add. Nothing in a module job changes production code except a seam a job above built.

## Jobs

| Job | Files | Waits on | Status |
| --- | --- | --- | --- |
| T1 | `tests/support/tolerance.hpp` copied from SushiSkills, and `expect_row` moved onto it |  | done |
| T2 | The collector seam |  | done |
| T3 | In-memory adapters beside `StreamWriter`, `Source`, `RecordWriter`, `EnvironmentContributor` |  | done |
| T4 | The CLI's `project` fixture replaced by sushicore's in-memory runner and console | sushicore T1 | done |
| T5 | The defects of this report, each with a test that fails first |  | done |
| T6 | `tools/testing/check_test_additions.py`, `tools/tests/test_check_test_additions.py` deleted |  | done |
| M1 | the tests of `capture` (18 files): 17 rewrite, 37 resize, 15 split, 5 delete | T1, T2, T3, T4, T5, T6 | done |
| M2 | the tests of `cli` (6 files): 23 rewrite, 1 resize, 2 merge | T1, T2, T3, T4, T5, T6 | todo |
| M3 | the tests of `cli/services` (1 file): 28 rewrite, 1 delete | T1, T2, T3, T4, T5, T6 | todo |
| M4 | the tests of `foundation` (10 files): 12 rewrite, 1 resize, 12 split, 4 delete | T1, T2, T3, T4, T5, T6 | todo |
| M5 | the tests of `package` (1 file): 1 resize | T1, T2, T3, T4, T5, T6 | todo |
| M6 | the tests of `record` (34 files): 24 rewrite, 9 split, 10 merge, 4 delete, 1 characterize | T1, T2, T3, T4, T5, T6 | todo |
| M7 | the tests of `session` (24 files): 66 rewrite, 6 resize, 26 split, 6 merge, 2 delete | T1, T2, T3, T4, T5, T6 | todo |
| M8 | the tests of `tools` (2 files): 1 rewrite, 2 resize, 1 split, 5 delete | T1, T2, T3, T4, T5, T6 | todo |
| M9 | the tests of `tools/record_tool` (1 file): 5 split | T1, T2, T3, T4, T5, T6 | todo |
| M10 | the tests of `tools/umbrella` (1 file): 1 rewrite | T1, T2, T3, T4, T5, T6 | todo |

## Resume note

M1 is done and pushed, with the orchestrator's three follow-ups of 2026-11-10 (`REPORT.md` decisions 8 to
10): the refused-count test, the `SP_LOCK_WAIT*_IN` macros, and the `Small_` condition-variable test. M2
(`cli`, 6 files) has not started as a job. Most of its `rewrite` records were already applied by T4:
`cli/tests/test_cli.py`, `test_env.py` and `test_package_contract.py` run on the `project` fixture's
recording runner and patch no repository function. What is left, file by file, in this order:

1. `cli/tests/test_cli.py`: merge `test_help_lists_the_four_commands` and `test_help_lists_the_docs_group`
   into one parametrised test over the command names.
2. `cli/tests/test_console.py`: `test_importing_the_console_builds_nothing` reads `console._lazy.built`
   (a private name) and needs a rewrite on the public surface of sushicore's `LazyConsole`
   (`D:/Projects/sushicore/source/sushicore/cli_console.py`, not read yet). `test_help_works_outside_a_checkout`
   is `medium` (it calls `monkeypatch.chdir`): put the two tests in two classes, one mark each, and drop the
   file-level `pytestmark`.
3. `cli/tests/test_env.py`: both tests are `medium` today (`os.environ`, a build folder). Open ruling: build
   an `environ` parameter on `load_build_env` and a `which` seam on `Config.resolved_compiler` (default
   unchanged) so they run `small`, or keep them `medium` and record why.
4. `test_config.py` and `test_main.py` are `keep`; check their size labels only.
5. Then the folder README of `cli/tests`, the `REPORT.md` decision for M2, and set M2 to done.

Syntax check: `syn.py` in the scratchpad (C++ only). Nothing built or ran: the first `sp test` over
`tests/capture` is still owed. A shared scratchpad is used by other agents: name message files with a
`prof_` prefix.

## Acceptance

- **T1.** No comparison with a tolerance spells a literal. Build: the repository's test command.
- **T2.** A small test records events with no process-wide collector. Build: the repository's test command.
- **T3.** Each keeps its port's promise; the session tests use them. Build: the repository's test command.
- **T4.** No CLI test patches the repository's own functions. Build: the CLI tests.
- **T5.** Every defect above is fixed or has a backlog entry. Build: the tests of each touched module.
- **T6.** Neither file exists and the checkers pass. Build: `python tools/check_all.py .`
- **M jobs.** Every record of the module is applied: no test the report deletes remains, every kept and
  new test passes at the size its label states, and the module's tests pass through the project's own
  test command. Build: that command over the module.

The owner chooses inline or parallel mode.
