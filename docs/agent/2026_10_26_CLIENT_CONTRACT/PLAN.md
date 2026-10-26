# SushiProfiler v0.2.0: plan

**Status:** Open — written 2026-10-26; C1 and C2 built and committed, C3 to C5 written and not built; the owner moved every build to the end of the programme.

Spec: `SPEC.md` in this folder. Every task closes on one `sp.sh build` and `sp.sh test`, all tests
passing, and `tools/checking/check_all.py` clean.

| Id | Task | Who | Acceptance |
| --- | --- | --- | --- |
| C1 | `SushiProfiler/version/export.hpp` with `SUSHIPROFILER_API` (dllexport, dllimport, visibility default, empty under `SUSHIPROFILER_STATIC`); `sushiprofiler` SHARED with `SUSHIPROFILER_EXPORTS`, hidden visibility, versioned SOVERSION `0.2`; `sushiprofiler_static` STATIC with `SUSHIPROFILER_STATIC` public; the package file exports both; tests link the shared one | orchestrator (build files) | The library builds shared on Windows and every test links against it |
| C2 | `SUSHIPROFILER_API` on every class and free function a header outside `detail/` declares and a source file defines; the collector's static gate reached through the exported class | worker | C1's build links with no unresolved symbol; a test in `tests/capture/collector/` proves a zone recorded through a second shared image (a test DLL) reaches the session the executable started |
| C3 | `PeriodSpan` in `sources`, `Source::collect(const PeriodSpan&, SpanSink&)`, the session passing each closed period's steady start and end; `SpanSink::drop_late(count)` into the period's dropped-span count; the `SourceContext` lifetime written in `source.hpp` and `sources/README.md` | worker | Tests: a source interns a name and adds a track inside `collect`; a span is placed in the period holding its start; work older than the given span is dropped and counted |
| C4 | The record keeps each active period's zone runs; `device_idle` attributes a gap to the innermost zone open on the submitting thread at the gap's start; the record format version and `RECORD_FORMAT.md` follow; `read_record` refuses the old version with its number in the message | worker | A test with one device span, a gap and a host zone open over the gap names that zone; a v0.1.0 record is refused by name |
| C5 | The guide page for a client library (`register_profiler_parts`, ownership, the shared or static choice), a test that registers a fake library through it, the RW-5 budget row, the design phase P2b, the changelog | worker | `check_docs_truth.py` clean; the guide's example is taken from the test file |
| C6 | Release v0.2.0 through `versioning-and-release` | orchestrator | Tag pushed after the owner's approval and a green CI run |
