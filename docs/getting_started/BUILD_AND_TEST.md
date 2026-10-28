# Building and testing

`sp` builds, tests and checks SushiProfiler. Install it from the checkout:

```bash
pip install -e ./cli
```

Every build and test goes through `sp`; do not call CMake, Ninja or CTest yourself.

In PowerShell `sp` is the built-in alias of `Set-ItemProperty`; type `sushiprofiler`, the same
command under its full name, there.

## sp build

Configures the tree when it needs it, then builds the library target, the tests and the header
checks.

```bash
sp build                      # debug, in build/default-debug/
sp build --type release       # its own tree, build/default-release/
sp build --sanitize address   # ASan and UBSan, in build/asan-debug/
sp build --sanitize thread    # ThreadSanitizer, in build/tsan-debug/
sp build -D SP_BUILD_TESTS=OFF
sp build --clean
sp build --report             # also times the edges and the memory the build used
```

Every build type has a tree of its own, so switching types never recompiles one. A configure
of a tree under another type than its own is refused. Each configure installs the memory gate
of sushicore, and the compile runs at the hardware thread count; `--report` prints the edges
per kind, the slowest ones and the lowest free memory, and writes `build_report.json` into the
tree. Benchmarks measure the type that ships: `sp build --benchmarks --type release`, then
`sp bench`.

## sp test

Builds when needed, then runs the tests. The quarantined ones are left out.

```bash
sp test                                # every size
sp test --label small --label medium   # what a pull request runs
sp test --filter 'Small_Version.*'     # ctest -R over Suite.Case
sp test --jobs 4                       # four tests at once
sp test --package                      # also the consumer of the installed package
```

`--package` installs the tree under its `package-prefix/` folder, builds `tests/package/` against it
and runs its one program.

## sp check

Runs nine of the shared checkers under `tools/`. `python -I tools/check_all.py .` runs all twelve,
with the backlog and design checkers, then the tests.

```bash
sp check
sp check --report    # counts per rule, exit 0
```

## sp record

Reads run records with the `sp_record` program that `sp build` builds. Each command exits 1 when
a record is invalid, a check fails or two builds differ, and 2 on a usage error or a file that
cannot be read. `sp record` itself exits 1 when the build holds no `sp_record`.

```bash
sp record validate run.sprec.json
sp record show run.sprec.json --analysis breakdown --top 10
sp record compare before.sprec.json after.sprec.json
sp record check run.sprec.json budgets.json
```

The check file is a JSON array of checks; its format is in
`modules/record/checks/README.md`.

## sp docs bundle

Writes the archive `docs.sushisystems.io` reads, in `build/docs/bundle`; `docs/publish.toml`
lists what goes in.

```bash
sp docs bundle --release 0.1.0
```

## Choosing the compiler

`sp` builds with `cxx` from `cli/config.local.toml` or `SP_CXX`, else a clang++ on PATH, else
CMake's platform default. On Windows a clang++ needs the Visual Studio environment: set
`vs_vcvars` to `vcvars64.bat`, or run `sp` from a developer prompt with `SP_CXX=cl`.
