# Changelog

## Unreleased

- 2026-10-27 — session: Fixed the SourceContext a source receives in begin(), which the session now keeps until every begun end() returns, so collect() may use it (`modules/session/session/`).
