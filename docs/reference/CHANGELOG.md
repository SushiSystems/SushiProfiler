# Changelog

## Unreleased

- 2026-10-26 — umbrella: Moved the umbrella module to the top tier, so it includes every module without an upward include (`modules/tools/umbrella/`).
- 2026-10-26 — sites: Added the site registry, which gives each recording site one id on first use and turns sites on and off by level and group at run time, and the name table (`modules/capture/sites/`).
- 2026-10-26 — events: Added the 24-byte trace event and its 13 kinds, with 64-bit times and value bits kept exactly (`modules/foundation/events/`).
- 2026-10-26 — clock: Added the trace clock, which reads the time-stamp counter under MSVC as under clang and calibrates it to steady time (`modules/foundation/clock/`).
- 2026-10-26 — repo: Created the repository skeleton with the sp CLI, the tier table, the version and umbrella modules and the documentation tree (`cli/`, `tools/`, `docs/`).
- 2026-10-26 — design: Opened the PROFILER topic, the design the owner approved for the stack's one profiler (`docs/design/PROFILER/`).
- 2026-10-26 — build: Stopped the post-build copy failing for a test program that loads no DLL, and kept extraction timestamps on the GoogleTest download (`cmake/`).
