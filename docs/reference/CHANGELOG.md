# Changelog

## v0.5.1 — 2026-11-22

- 2026-11-22 — cli: Required SushiCore 0.16, the release the stack's CLIs share (`cli/pyproject.toml`).
- 2026-11-22 — cli: Fixed `sp test --sanitize`, which judged the time limits; it hands the sanitizer to the shared test command, which judges none under a sanitizer (`cli/sushiprofiler/services/project.py`).
