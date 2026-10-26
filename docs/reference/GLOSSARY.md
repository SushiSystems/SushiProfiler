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
| Window | The part of a timeline that is recorded, chosen by a schedule over its periods; `SushiProfiler/window/window.hpp` |
| Source | A provider of measurements that the profiler does not take itself, such as a device that reports its own spans; `SushiProfiler/sources/source.hpp` |
| Span | A start and an end on the profiler's clock, reported by a source and tied to the period it belongs to |
| Record | What one run leaves: its periods, its measurements and the facts about the machine and the program, kept as a `.sprec.json` file; `SushiProfiler/record/run_record.hpp` |
| Session | One recording run of a program, from a configuration to a record; one runs per process; `SushiProfiler/session/session.hpp` |
| Configuration | What a session records and writes: the timeline, the window, the sources, the analyses and the writers; `SushiProfiler/config/profiler_config.hpp` |
| Consumer | The thread that drains the rings and closes periods in order; `SushiProfiler/consumer/consumer.hpp` |
| Linger | How long a period stays open after its end, so that events drained late still join it |
| Late | An event that begins in a period already closed; counted per thread, not recorded |
| Unmatched | A zone or lane end with no open begin, or a zone that crosses its parent; counted per thread |
| Fold | Turning a period's events into one zone tree per thread and lane, with self time from the children; `SushiProfiler/fold/period_fold.hpp` |
| Analysis | A question asked of a record, answered as a table; `SushiProfiler/analysis/analysis.hpp` |
| Subject | A named series of samples in a record, such as `timeline:train.step` or `zone:forward`, that checks and comparisons read; `SushiProfiler/analysis/subject_samples.hpp` |
| Check | A bound on one statistic of one subject that passes or fails, such as a p99 budget for a period; `SushiProfiler/checks/check.hpp` |
| Verdict | A check's result on one record: passed or not, the value and the reason; `SushiProfiler/checks/verdict.hpp` |
| Stream writer | A writer given each period as it closes, such as the Chrome trace; `SushiProfiler/stream_writers/stream_writer.hpp` |
| Record writer | A writer given the finished record and its analyses, such as the terminal summary; `SushiProfiler/record_writers/record_writer.hpp` |
| Registry | The factories of sources and writers by name, and the environment contributors; `SushiProfiler/registry/registry.hpp` |
