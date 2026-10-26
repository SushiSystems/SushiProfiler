# Architecture

SushiProfiler is a C++17 library in the namespace `SushiProfiler`. It depends on the C++
standard library and Threads, and on nothing else; GoogleTest builds the tests only. This page
describes the structure as it is. The modules the library will gain, and what they hold, are in
the design documents under `docs/design/`.

## Tiers

Modules sit in tiers, lowest first. `cmake/SushiProfilerTiers.cmake` states the order once, in
`SUSHIPROFILER_TIER_ORDER`; `tools/settings.py` holds the layering checker's copy.

| Tier | Modules today |
| --- | --- |
| `foundation` | `version`, `clock`, `events` |
| `capture` | `sites`, `rings` |
| `record` | none |
| `session` | none |
| `tools` | `umbrella` |

A tier is the folder `modules/<tier>/`; it appears with its first module.

## The dependency rule

A module includes only modules of a lower tier, and only those it lists in its `DEPENDS`. A lower
tier never includes a higher one. Two layers hold it.

- The build. `sushiprofiler_add_module` in `cmake/SushiProfilerModule.cmake` refuses a dependency
  that is not declared before its consumer, that the tier table does not list, or that is not
  below the consumer's tier. A module's include roots are those of its own and its listed
  dependencies, so an include of any other module does not compile. Each public header is also
  compiled alone.
- The checker. `tools/layering/check_layering.py` reads the same order and fails an upward
  include, an include of another module's `source/`, and an include the module did not declare.

The module `umbrella` sits in the top tier, so every module is below it, and it includes every
public header of the library.

## Adding a module

A module is a folder `modules/<tier>/<module>/` with `include/SushiProfiler/<module>/`, a
`README.md` and a `CMakeLists.txt` that calls `sushiprofiler_add_module`, plus one line in
`SUSHIPROFILER_MODULE_TIERS`. Its tests go in `tests/<tier>/<module>/`, whose `CMakeLists.txt`
calls `sushiprofiler_add_module_tests`. A dependency is listed before its consumer in the table.
Neither `modules/CMakeLists.txt` nor `tests/CMakeLists.txt` changes: both walk the table.

## The library target

`sushiprofiler_compose_library` in `cmake/SushiProfilerLibrary.cmake` builds the one target,
`SushiProfiler::SushiProfiler`, from what the modules recorded. While no module lists a source
file the target is an INTERFACE library carrying the include roots of every module and Threads.
Once a module lists sources, it is a static library over them. `sushiprofiler_install_package`
installs it as the package `SushiProfiler`; `tests/package/` builds a consumer against that
install.

## The version

`modules/foundation/version/` holds the library version in `version.hpp`. Its `CMakeLists.txt`
stops the configure step when that number differs from the one `project()` declares in the root
`CMakeLists.txt`.
