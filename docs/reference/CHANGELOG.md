# Changelog

## Unreleased

- 2026-10-30 — build: Fixed the CI checkers, the package test's Debug import and the sanitizer builds of the allocation counter (`ci.yml`, `tests/`).
- 2026-10-29 — build: Let a Debug tree link an imported package built as `None`, such as Ubuntu's GTest (`cmake/`).
## v0.4.1 — 2026-10-29

- 2026-10-29 — version: Set `version.hpp` to the version `project()` declares, so the v0.4.0 configure no longer stops on the mismatch (`version.hpp`).
