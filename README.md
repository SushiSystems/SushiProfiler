# SushiProfiler

SushiProfiler is a C++17 profiler for the Sushi libraries and for any other C++17 program: it
records where the time of a marked period of the program goes. It depends on the C++ standard
library and Threads only.

A program starts a `Session` from a configuration, marks its periods and records zones, counters
and flows; `modules/session/session/README.md` shows the calls. The session leaves a run record,
a terminal summary and a Chrome trace, and the record is read back with `sp`:

```bash
sp record show build/profiles/run.sprec.json --analysis breakdown --top 10
sp record compare before.sprec.json after.sprec.json
sp record check run.sprec.json budgets.json
```

## Build

Standalone, with Git, Python 3.11, CMake and a C++17 compiler:

```bash
git clone https://github.com/SushiSystems/SushiProfiler.git
cd SushiProfiler
pip install -e ./cli
sp build --type release
sp test --label small --label medium --package
```

With SushiHub: `hub add sushiprofiler` and `hub install-cli sushiprofiler`, or
`hub link sushiprofiler <path>` for a checkout you already have.
[`docs/getting_started/INSTALL.md`](docs/getting_started/INSTALL.md) has the prerequisites for
both.

In PowerShell `sp` is a built-in alias; type `sushiprofiler` there. Every build and test goes
through `sp`.

## Status

Design phases P1 and P2 are built: the capture layer, the session that turns a window of periods
into a run record, the analyses, comparison and checks over a record, the Chrome trace, and the
record tool behind `sp record`. No library of the stack records through it yet; SushiRuntime's
device source is phase P3. The open work is in
[`docs/backlog/README.md`](docs/backlog/README.md).

## Documentation

The manual starts at [`docs/README.md`](docs/README.md); contributors read
[`docs/CONTRIBUTING.md`](docs/CONTRIBUTING.md).

## Licence

SushiProfiler is source-available under the PolyForm Noncommercial License 1.0.0; see
[`LICENSE`](LICENSE), [`COMMERCIAL.md`](COMMERCIAL.md) and [`NOTICE.md`](NOTICE.md).
