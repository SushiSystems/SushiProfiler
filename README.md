# SushiProfiler

SushiProfiler is a C++17 profiler for the Sushi libraries and for any other C++17 program: it
records where the time of a marked period of the program goes. It depends on the C++ standard
library and Threads only.

There is no library API yet. What a developer types today is the `sp` CLI:

```bash
sp --help
sp check
```

## Build

```bash
pip install -e ./cli
sp build --type release
sp test --label small --label medium --package
```

In PowerShell `sp` is a built-in alias; type `sushiprofiler` there. Every build and test goes
through `sp`.

## Status

The capture layer (design phase P1) is being built: the clock, the event types, the site
registry, the rings, the collector, the recording macros and lock waits exist; periods do not
yet, and nothing above the capture tier is written. The open work is in
[`docs/design/REMAINING_WORK.md`](docs/design/REMAINING_WORK.md).

## Documentation

The manual starts at [`docs/README.md`](docs/README.md); contributors read
[`docs/CONTRIBUTING.md`](docs/CONTRIBUTING.md).

## Licence

SushiProfiler is source-available under the PolyForm Noncommercial License 1.0.0; see
[`LICENSE`](LICENSE), [`COMMERCIAL.md`](COMMERCIAL.md) and [`NOTICE.md`](NOTICE.md).
