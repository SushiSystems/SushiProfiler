# cli

The `sp` command line interface. `sushiprofiler/cli.py` parses the four commands, `build`, `test`,
`bench` and `check`, and the `record` group, registers the shared `package` command, and `sushiprofiler/services/project.py` does the work. The `docs` group, with its one command
`bundle`, the config loader, the console, the process runner and the CMake driver come from
sushicore, at the version `pyproject.toml` pins. `tests/` is the pytest suite:
`cd cli && python -m pytest tests -q`.

## config.py

Configuration is layered, lowest precedence first: built-in defaults, `config.toml`,
`config.local.toml`, then `SP_*` environment variables such as `SP_CXX`. SushiProfiler pins no
toolchain: `sp` builds with the pinned `cxx`, else a clang++ on PATH, else CMake's platform
default. A found compiler with a found ninja takes the Ninja generator; with no compiler CMake
chooses its own.

Inside a SushiHub workspace the workspace's shared configuration is layered into this one. The
workspace is the folder `SUSHISTACK_HOME` names, else the one the module's `[link]` pointer
names, else the nearest parent holding `.sushistack`. A standalone checkout has none and reads
only its own files.

## host.py

The `Host` port is how `sp` reads the environment and finds an executable: `environment()` (a copy),
`variable(name)` and `which(name)`. `host_system.py` answers it from `os.environ` and `shutil.which`;
`host_memory.py` answers it from two maps, for tests. `Config.resolved_compiler`, `resolved_ninja`,
`resolved_generator` and `is_multi_config`, `load_config` and `env.load_build_env` take a `host`; none
means the system one.

## Package contract

SushiCore builds a SushiProfiler package for a consumer by running `sp build --type <type>
--build-dir build/pkg-<id> --lean` and then `sp package --build-dir build/pkg-<id> --prefix <p>
--no-archive --force` in this checkout. `--build-dir` names the tree below the checkout. `--lean`
builds no tests, applications or benchmarks (`SP_BUILD_TESTS`, `SP_BUILD_APPLICATIONS` and
`SP_BUILD_BENCHMARKS` off) and refuses `--benchmarks`. `build` always passes the tests and the
applications switches, so a tree switches between the two by reconfiguring. `package` is
sushicore's shared command. `SUSHISTACK_CXX` names a compiler that outranks the configured one.
`cli/sushistack.deps.toml` names this CLI in `[module] cli`.

## console.py

The console is built on first use, through sushicore's `LazyConsole`. Building it needs this
repository's `cli/` folder, so building it at import time would make `sp --help` fail outside a
checkout. `wired()` returns that `LazyConsole` without building it, so a test can check what the
module wires.

## env.py

With a compiler found, cmake and ctest run under the environment of the host (the process's), with the compiler's
folder first on PATH and, on Windows with `vs_vcvars` set, the Visual Studio environment that
`vcvars64.bat` produces, cached in the build tree.

## services/project.py

Debug is the default type; `sp bench` runs the release one. A tree is `build/<name>-<type>`,
with the names `default`, `asan` and `tsan`, so each sanitizer and each type has a tree of its own
and switching rebuilds nothing. A configure refuses a tree configured as another type, and a
`-D CMAKE_BUILD_TYPE`; it also refuses a tree whose builds the owner stopped, and installs the
memory gate of sushicore (`build_admission`) with the compiler cache. The compile runs at the
hardware thread count. `sp build --report` prints and writes the report of the build. `sp test` leaves the quarantined tests out. `sp test --package`
installs the tree, builds `tests/package/` against the install and runs its test. `sp build
--benchmarks` turns on `SP_BUILD_BENCHMARKS`, and `sp bench` runs every `sp_bench_*` program in
`build/default-release/benchmarks/Release/` in name order. `sp record show|compare|check|validate` runs `sp_record` from
`build/default-<type>/applications/<Config>/` in the current folder, so relative record paths work, and exits
with its code. `sp check` runs the nine shared checkers and, with
`--rule`, only those whose `--list-rules` names the rule.
Each of `build`, `test`, `bench`, `record` and `check` takes a `Workspace`: the root, the configuration,
the console, the command runner and, when given, the environment the commands run under. `sp`
passes the checkout it runs in; a test passes a scratch checkout over sushicore's `RecordingRunner`
and `RecordingConsole`, which start nothing.
