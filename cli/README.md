# cli

The `sp` command line interface. `sushiprofiler/cli.py` parses the four commands, `build`, `test`,
`bench` and `check`, and `sushiprofiler/services/project.py` does the work. The `docs` group, with its one command
`bundle`, the config loader, the console, the process runner and the CMake driver come from
sushicore 0.9.2. `tests/` is the pytest suite:
`cd cli && python -m pytest tests -q`.

## config.py

Configuration is layered, lowest precedence first: built-in defaults, `config.toml`,
`config.local.toml`, then `SP_*` environment variables such as `SP_CXX`. SushiProfiler pins no
toolchain: `sp` builds with the pinned `cxx`, else a clang++ on PATH, else CMake's platform
default. A found compiler with a found ninja takes the Ninja generator; with no compiler CMake
chooses its own.

## console.py

The console is built on first use, through sushicore's `LazyConsole`. Building it needs this
repository's `cli/` folder, so building it at import time would make `sp --help` fail outside a
checkout.

## env.py

With a compiler found, cmake and ctest run under the current environment, with the compiler's
folder first on PATH and, on Windows with `vs_vcvars` set, the Visual Studio environment that
`vcvars64.bat` produces, cached in the build tree.

## services/project.py

Each sanitizer has its own build tree: `build/`, `build-asan/` and `build-tsan/`, so switching
does not rebuild the plain tree. `sp test` leaves the quarantined tests out. `sp test --package`
installs the tree, builds `tests/package/` against the install and runs its test. `sp build
--benchmarks` turns on `SP_BUILD_BENCHMARKS`, and `sp bench` runs every `sp_bench_*` program in
`build/benchmarks/<config>/` in name order. `sp check` runs the nine shared checkers and, with
`--rule`, only those whose `--list-rules` names the rule.
