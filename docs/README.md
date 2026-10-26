# SushiProfiler manual

SushiProfiler is a profiler for the Sushi libraries and any other C++17 program. Design phases P1
and P2 are built: the capture layer, the session, the run record, its analyses, comparison and
checks, and the record tool behind `sp record`.

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
- The tests: [`tests`](../tests/README.md); the CLI: [`cli`](../cli/README.md); the checkers:
  [`tools`](../tools/README.md); the programs: [`applications`](../applications/README.md) and
  [`sp_record`](../applications/sp_record/README.md).

## Modules

- foundation: [`clock`](../modules/foundation/clock/README.md), [`events`](../modules/foundation/events/README.md), [`json`](../modules/foundation/json/README.md), [`version`](../modules/foundation/version/README.md)
- capture: [`collector`](../modules/capture/collector/README.md), [`lock_wait`](../modules/capture/lock_wait/README.md), [`periods`](../modules/capture/periods/README.md), [`rings`](../modules/capture/rings/README.md), [`sites`](../modules/capture/sites/README.md), [`zones`](../modules/capture/zones/README.md)
- record: [`analysis`](../modules/record/analysis/README.md), [`checks`](../modules/record/checks/README.md), [`compare`](../modules/record/compare/README.md), [`record`](../modules/record/record/README.md), [`record_json`](../modules/record/record_json/README.md), [`record_writers`](../modules/record/record_writers/README.md), [`statistics`](../modules/record/statistics/README.md)
- session: [`config`](../modules/session/config/README.md), [`consumer`](../modules/session/consumer/README.md), [`environment`](../modules/session/environment/README.md), [`fold`](../modules/session/fold/README.md), [`registry`](../modules/session/registry/README.md), [`session`](../modules/session/session/README.md), [`sources`](../modules/session/sources/README.md), [`stream_writers`](../modules/session/stream_writers/README.md), [`window`](../modules/session/window/README.md)
- tools: [`record_tool`](../modules/tools/record_tool/README.md), [`umbrella`](../modules/tools/umbrella/README.md)
