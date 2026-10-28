# Install SushiProfiler

SushiProfiler installs in two ways. Both give the same checkout and the same `sp` command, and
neither needs the other: SushiHub is not required to build SushiProfiler, and a checkout made
without it joins a SushiHub workspace later without any change.

## Standalone

You need:

- Git, Python 3.11 or later and CMake 3.21 or later.
- A C++17 compiler. `sp` takes `cxx` from its configuration, else a clang++ on the path, else
  the compiler CMake picks; [`BUILD_AND_TEST.md`](BUILD_AND_TEST.md), "Choosing the compiler",
  has the Windows detail.
- Ninja, optional. `sp` uses it when it finds one.
- Network access once, for the googletest release that the tests fetch. `-D SP_BUILD_TESTS=OFF`
  builds the library without it.

SushiProfiler needs no sibling repository and no library of the stack.

```bash
git clone https://github.com/SushiSystems/SushiProfiler.git
cd SushiProfiler
pip install -e ./cli
sp build --type release
sp test --label small --label medium --package
```

`pip install` also installs `sushicore`, the command-line layer `sp` shares with the other Sushi
modules, from PyPI at the version `cli/pyproject.toml` pins. In PowerShell `sp` is a built-in
alias; type `sushiprofiler` there.

## With SushiHub

```bash
hub add sushiprofiler            # clone it into the workspace
hub install-cli sushiprofiler    # put sp on the path
hub link sushiprofiler <path>    # adopt a checkout you already have, instead of hub add
```

`hub link` records where the checkout is and changes nothing in it. Inside a workspace `sp` also
reads the workspace's shared configuration; [`cli/README.md`](../../cli/README.md), "config.py",
says how it finds the workspace. After either route, `sp build` and `sp test` are the commands
above.
