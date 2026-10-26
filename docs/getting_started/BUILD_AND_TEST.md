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
sp build                      # the type the tree carries, else debug
sp build --type release
sp build --sanitize address   # ASan and UBSan, in build-asan/
sp build --sanitize thread    # ThreadSanitizer, in build-tsan/
sp build -D SP_BUILD_TESTS=OFF
sp build --clean
```

## sp test

Builds when needed, then runs the tests. The quarantined ones are left out.

```bash
sp test                                # every size
sp test --label small --label medium   # what a pull request runs
sp test --filter 'Small_Version.*'     # ctest -R over Suite.Case
sp test --jobs 4                       # four tests at once
sp test --package                      # also the consumer of the installed package
```

`--package` installs the tree under `build/package-prefix/`, builds `tests/package/` against it
and runs its one program.

## sp check

Runs the shared checkers under `tools/`.

```bash
sp check
sp check --report    # counts per rule, exit 0
```

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
