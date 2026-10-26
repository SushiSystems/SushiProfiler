# SushiProfiler manual

SushiProfiler is the profiler of the Sushi libraries. Only the repository skeleton exists so far;
the pages below describe it as it is.

## Getting started

- [`getting_started/BUILD_AND_TEST.md`](getting_started/BUILD_AND_TEST.md): installing `sp`,
  building, testing and checking.

## Architecture

- [`architecture/ARCHITECTURE.md`](architecture/ARCHITECTURE.md): the tiers, the dependency
  rule and the library target.

## Reference

- [`reference/CHANGELOG.md`](reference/CHANGELOG.md)
- [`reference/KNOWN_ISSUES.md`](reference/KNOWN_ISSUES.md)
- [`reference/GLOSSARY.md`](reference/GLOSSARY.md)
- [`reference/CONFIG_FORMAT.md`](reference/CONFIG_FORMAT.md): the profiler configuration's JSON form, key by key
- [`reference/RECORD_FORMAT.md`](reference/RECORD_FORMAT.md): the run record's JSON form, key by key

## Working on SushiProfiler

- [`CONTRIBUTING.md`](CONTRIBUTING.md): what a change contains and how it lands.
- [`DOCUMENTATION_STYLE_GUIDE.md`](DOCUMENTATION_STYLE_GUIDE.md)
- [`design/README.md`](design/README.md) and [`design/REMAINING_WORK.md`](design/REMAINING_WORK.md)
- The modules: [`version`](../modules/foundation/version/README.md) and
  [`umbrella`](../modules/tools/umbrella/README.md); the tests: [`tests`](../tests/README.md);
  the CLI: [`cli`](../cli/README.md); the checkers: [`tools`](../tools/README.md).
