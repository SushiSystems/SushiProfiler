# Glossary

The words this repository's documents use. No header defines them yet; each entry gives the
meaning the design intends, and a term that code defines will name the header that holds it.

| Word | Meaning |
| --- | --- |
| Zone | A stretch of code a program marks, so that the time it takes is measured and recorded |
| Site | The place in the source where a zone is declared; every execution of that place shares one site |
| Period | A stretch of the program's own work that the program marks, such as a training step or a frame; the unit a measurement answers for |
| Timeline | The ordered sequence of the periods one program marks |
| Window | The part of a timeline that is recorded, chosen by a schedule over its periods |
| Source | A provider of measurements that the profiler does not take itself, such as a device that reports its own spans |
| Span | A start and an end on the profiler's clock, reported by a source and tied to the period it belongs to |
| Record | The file one run leaves: its periods, its measurements and the facts about the machine and the program |
| Check | A rule over a record that passes or fails, such as a time budget for a period |
