# record_tool

The record tool, in the namespace `SushiProfiler`: show, compare, check and validate run record
files from the command line. The program `applications/sp_record/` runs it, and `sp record`
runs that program. The module depends on `analysis`, `checks`, `compare`, `record`,
`record_json` and `record_writers`.

- `SushiProfiler/record_tool/record_tool.hpp`: `run_record_tool(arguments, out, err)` runs one
  command on the arguments after the program name, writes results to `out` and refusals to
  `err`, and returns `K_TOOL_OK` (0), `K_TOOL_FAILED` (1) or `K_TOOL_USAGE` (2).

| Command | Does | Fails with 1 when |
| --- | --- | --- |
| `show <record> [--analysis NAME]... [--top N] [--group-by KEY]... [--baseline RECORD]` | Runs the named analyses and prints their tables; with no `--analysis`, `breakdown`, `distribution`, `counters`, `flows`, `top_spans` and `overhead` | The record is invalid |
| `compare <baseline> <candidate> [--allow-build-mismatch]` | Prints every row of `compare_records` | A record is invalid, or the builds differ and the mismatch is not allowed |
| `check <record> <checks>` | Prints `pass` or `FAIL` per check, then `N of M checks passed` | A record is invalid, or a check fails |
| `validate <record>` | Reads the record strictly and prints its period and thread counts | The record is invalid |
| `help` | Prints the usage | Never |

Exit code 2 means the tool could not do what was asked: an unknown command, option or analysis,
a missing value, a wrong number of files, a file that cannot be read, or a check file that does
not parse. Every refusal names the file it comes from.

`source/tool_arguments.hpp` splits the arguments of a command into words, options with a value
and flags, and reads the files. Each command is its own source file behind
`source/record_commands.hpp`, and `record_tool.cpp` finds a command by its word in one table.

The tests are in `tests/tools/record_tool/`.
