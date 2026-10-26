# Glossary

The words this repository's documents use. A word the code defines names the header that holds
it; the others give the meaning the design intends.

| Word | Meaning |
| --- | --- |
| Zone | A stretch of code a program marks, so that the time it takes is recorded as one event; `SushiProfiler/zones/zone.hpp` |
| Site | The place in the source where a record is declared; every execution of that place shares one site and one id; `SushiProfiler/sites/site.hpp` |
| Collector | The one process-wide object: the gate, every thread's ring and the drain one consumer polls; `SushiProfiler/collector/collector.hpp` |
| Gate | The collector's open or closed state; a closed gate makes every record a single load and nothing more |
| Ring | One thread's lock-free buffer of events between its records and the consumer's drain; a full ring drops and counts; `SushiProfiler/rings/thread_ring.hpp` |
| Lane | A scope on one thread whose events form a row of their own, for a thread that serves several streams |
| Flow | The path of one item across threads, written as a begin, steps and an end that share the item's id |
| Period | A stretch of the program's own work that the program marks, such as a training step or a frame; the unit a measurement answers for |
| Timeline | The numbered sequence of the periods one name marks; `SushiProfiler/periods/period_timeline.hpp` |
| Window | The part of a timeline that is recorded, chosen by a schedule over its periods |
| Source | A provider of measurements that the profiler does not take itself, such as a device that reports its own spans |
| Span | A start and an end on the profiler's clock, reported by a source and tied to the period it belongs to |
| Record | The file one run leaves: its periods, its measurements and the facts about the machine and the program |
| Check | A rule over a record that passes or fails, such as a time budget for a period |
