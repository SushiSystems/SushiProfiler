# Writing a client library

A client library is a library that reports its own zones, or work measured on a device, through
SushiProfiler. This page states the three choices it makes: which SushiProfiler target it links,
how it registers its parts, and what its source may rely on.

## Shared or static

Link `SushiProfiler::SushiProfiler`, the shared library. A process then has one collector, because
every image that records through the zone macros reaches the same `Collector`, and a host sees the
zones of every library it loads.

Link `SushiProfiler::SushiProfilerStatic` only for a plug-in that must keep a collector of its own,
so that its zones never reach the host's. The static target sets `SUSHIPROFILER_STATIC` for
whoever links it, which turns the export macro off.

A library and its host link the same target. A library on the static target inside a host on the
shared one records into a collector the host never drains.

## Registering the parts

A library exports one function that adds everything it offers to a `Registry` and returns the
object that owns what the registry only refers to. Its name is `register_profiler_parts`, and its
return type is `<Lib>ProfilerParts`:

```cpp
[[nodiscard]] FakeDeviceProfilerParts register_profiler_parts(Registry& registry)
{
    return FakeDeviceProfilerParts(registry);
}
```

Source: `tests/session/registry/test_registry.cpp`

The registry holds a source as a factory, and the session calls it to make the source and owns the
result. It holds a contributor by reference. The parts object therefore owns what the source reads
and the contributor itself, and it registers both in its constructor:

```cpp
explicit FakeDeviceProfilerParts(Registry& registry)
{
    const auto make_source = [this](const JsonValue& settings)
    {
        static_cast<void>(settings);
        return Made<Source>{std::make_unique<FakeDeviceSource>(queue_), ""};
    };
    registry.add_source("fake_device", make_source);
    registry.add_contributor(contributor_);
}
```

Source: `tests/session/registry/test_registry.cpp`

Three rules follow from this shape.

- The parts object is neither copyable nor movable, because the factory and the registry hold its
  address. The function returns it as a prvalue, so the caller's variable is the object itself.
- The caller keeps the parts object alive until every session started with the registry has
  stopped. The sources those sessions made read the queue it owns.
- A name is registered once. `add_source` returns false for a name that is taken.

## What a source may rely on

The session calls `begin`, `collect` and `end` on its consumer thread. A library that receives its
data on other threads queues it, as the example's queue does, and hands it over in `collect`.

The `SourceContext` that `begin` receives stays valid until `end` returns. `collect` may call
`intern`, `add_track` and `to_record_ns` on it, so a name that only exists once work arrives can
still be interned. No lock is needed.

`collect` receives a `PeriodSpan`: the period's index and its start and end on the steady clock.
A source places work in the period whose span holds the work's start, keeps work that starts
later for a later call, and calls `SpanSink::drop_late` for work that started before the span, which
counts it in the record's dropped spans:

```cpp
void collect(const PeriodSpan& period, SpanSink& sink) override
{
    for (const std::uint64_t start : queue_.take())
        pending_.push_back(start);
    for (auto start = pending_.begin(); start != pending_.end();)
    {
        if (*start >= period.end_steady_ns)
        {
            ++start;
            continue;
        }
        if (*start < period.start_steady_ns)
        {
            sink.drop_late(1U);
        }
        ...
```

Source: `tests/session/registry/test_registry.cpp`

How long a closed period waits for late work is the configuration's `buffers.linger_ms`. A source
whose work arrives later than that states it in `Source::max_delay_ns()`, and every period then
waits for the largest delay; the session refuses a delay above 60 000 ms. A source sets a fact on
the period it fills with `SpanSink::set_fact(name, value)`, and reads `Collector::sources_wanted()`
to pay for device timing only while a session is recording: it is true from the session's start of
its sources until their end, except while the session is paused.
