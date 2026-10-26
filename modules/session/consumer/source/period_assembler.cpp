/**************************************************************************/
/* period_assembler.cpp                                                   */
/**************************************************************************/
/*                          This file is part of:                         */
/*                              SushiProfiler                             */
/*              https://github.com/sushisystems/sushiprofiler             */
/*                         https://sushisystems.io                        */
/**************************************************************************/
/* Copyright (c) 2026-present Mustafa Garip & Sushi Systems               */
/*                                                                        */
/* Licensed under the PolyForm Noncommercial License 1.0.0 (the           */
/* "License"); you may not use this file except in compliance with the    */
/* License. You may obtain a copy of the License at                       */
/*                                                                        */
/*     https://polyformproject.org/licenses/noncommercial/1.0.0           */
/*                                                                        */
/* Noncommercial use is free. Commercial use requires a separate licence  */
/* from Sushi Systems; see COMMERCIAL.md. The software is provided        */
/* "as is", without warranty of any kind.                                 */
/**************************************************************************/

/**
 * @file period_assembler.cpp
 * @brief Opens periods on marks, places waiting events by begin time and closes periods in order.
 * @author Mustafa Garip
 * @see modules/session/consumer/README.md
 */

#include <SushiProfiler/consumer/period_assembler.hpp>

#include <algorithm>
#include <limits>
#include <tuple>

namespace SushiProfiler
{
    namespace
    {
        /// @brief A mark or budget of the followed timeline, with where it came from.
        struct PeriodEvent
        {
            std::uint64_t time_ns = 0U; /**< When it was written, on the record's clock. */
            std::uint32_t thread = 0U;  /**< The writing thread. */
            std::size_t order = 0U;     /**< Its place in that thread's drain. */
            TraceEvent event{};         /**< The event. */
        };

        /// @brief Returns whether @p left was written before @p right.
        bool written_before(const PeriodEvent& left, const PeriodEvent& right) noexcept
        {
            return std::tie(left.time_ns, left.thread, left.order) <
                   std::tie(right.time_ns, right.thread, right.order);
        }

        /// @brief Returns whether events of @p kind never join a period's events.
        bool is_control(EventKind kind) noexcept
        {
            return kind == EventKind::PeriodMark || kind == EventKind::PeriodBudget ||
                   kind == EventKind::LaneBegin || kind == EventKind::LaneEnd;
        }
    } // namespace

    PeriodAssembler::PeriodAssembler(const AssemblySettings& settings, RunRecord& record,
                                     const NameTable& names, const SiteRegistry& sites,
                                     std::vector<Source*> sources,
                                     std::vector<NamedStreamWriter> writers)
        : settings_(settings), record_(record), names_(names), sites_(sites),
          sources_(std::move(sources)), writers_(std::move(writers)),
          calibration_(settings.origin, settings.origin)
    {
        if (settings_.timeline != 0U)
            return;
        OpenPeriod run;
        run.period.timeline = settings_.run_timeline;
        open_.push_back(std::move(run));
    }

    void PeriodAssembler::add(const DrainBatch& batch)
    {
        if (batch.ticks_are_tsc && batch.point.ticks != settings_.origin.ticks)
            calibration_ = Calibration(settings_.origin, batch.point);
        const RecordClock clock(calibration_, settings_.origin.steady_ns, batch.ticks_are_tsc);
        now_ns_ = std::max(now_ns_, clock.from_steady(batch.point.steady_ns));
        record_.clock.source = batch.ticks_are_tsc ? "tsc" : "steady";
        record_.clock.nanoseconds_per_tick =
            batch.ticks_are_tsc ? calibration_.nanoseconds_per_tick() : 1.0;
        for (const ThreadDrain& drain : batch.threads)
        {
            ThreadRecord& thread = thread_record(drain.thread_index);
            if (!drain.name.empty())
                thread.name = drain.name;
            thread.dropped += drain.dropped;
        }
        read_marks(batch, clock);
        for (const ThreadDrain& drain : batch.threads)
            queue(drain, clock);
        const std::uint64_t horizon =
            now_ns_ > settings_.linger_ns ? now_ns_ - settings_.linger_ns : 0U;
        place_waiting(horizon);
        close_ended(horizon);
    }

    void PeriodAssembler::finish()
    {
        constexpr std::uint64_t K_END_OF_TIME = std::numeric_limits<std::uint64_t>::max();
        place_waiting(K_END_OF_TIME);
        if (settings_.timeline == 0U)
        {
            for (OpenPeriod& open : open_)
            {
                if (!open.ended)
                {
                    open.ended = true;
                    open.period.end_ns = std::max(now_ns_, open.period.begin_ns);
                }
            }
        }
        close_ended(K_END_OF_TIME);
        for (const OpenPeriod& open : open_)
        {
            for (const auto& entry : open.events)
                record_.drops.outside_periods += entry.second.size();
        }
        open_.clear();
        for (const auto& [thread, state] : threads_)
        {
            record_.drops.open_at_stop += state.lanes.open_lanes();
            thread_record(thread).unmatched += state.lanes.unmatched();
        }
        copy_tables();
    }

    const std::vector<std::string>& PeriodAssembler::problems() const noexcept
    {
        return problems_;
    }

    void PeriodAssembler::read_marks(const DrainBatch& batch, const RecordClock& clock)
    {
        if (settings_.timeline == 0U)
            return;
        std::vector<PeriodEvent> marks;
        for (const ThreadDrain& drain : batch.threads)
        {
            for (std::size_t order = 0U; order < drain.events.size(); ++order)
            {
                const TraceEvent& event = drain.events[order];
                const EventKind kind = kind_of(event);
                const bool marking =
                    kind == EventKind::PeriodMark || kind == EventKind::PeriodBudget;
                if (marking && event.name == settings_.timeline)
                {
                    marks.push_back(PeriodEvent{clock.from_ticks(event.begin), drain.thread_index,
                                                order, event});
                }
            }
        }
        std::sort(marks.begin(), marks.end(), written_before);
        for (const PeriodEvent& entry : marks)
        {
            if (kind_of(entry.event) == EventKind::PeriodMark)
            {
                mark(entry.event.end, entry.thread, entry.time_ns);
                continue;
            }
            if (!open_.empty() && !open_.back().ended)
                open_.back().period.budget_ns = entry.event.end;
        }
    }

    void PeriodAssembler::mark(std::uint64_t index, std::uint32_t thread, std::uint64_t time_ns)
    {
        bool was_open = false;
        for (OpenPeriod& open : open_)
        {
            if (!open.ended)
            {
                open.ended = true;
                open.period.end_ns = time_ns;
                was_open = true;
            }
        }
        if (settings_.window != nullptr && !settings_.window->records(index))
            return;
        if (!was_open)
            reopened_ns_.push_back(time_ns);
        OpenPeriod next;
        next.period.index = index;
        next.period.timeline = settings_.timeline;
        next.period.thread = thread;
        next.period.begin_ns = time_ns;
        next.period.warmup = settings_.window != nullptr &&
                             settings_.window->phase_of(index) == WindowPhase::Warmup;
        open_.push_back(std::move(next));
    }

    void PeriodAssembler::queue(const ThreadDrain& drain, const RecordClock& clock)
    {
        ThreadState& state = threads_[drain.thread_index];
        for (const TraceEvent& raw : drain.events)
        {
            const TraceEvent event = clock.to_record(raw);
            const EventKind kind = kind_of(event);
            const bool zone = kind == EventKind::Zone || kind == EventKind::ExternalZone;
            const std::uint64_t written_ns = zone ? event.end : event.begin;
            for (const std::uint64_t reopened : reopened_ns_)
            {
                if (state.last_write_ns < reopened && reopened <= written_ns)
                {
                    state.lanes.reset();
                    break;
                }
            }
            state.last_write_ns = std::max(state.last_write_ns, written_ns);
            const NameID lane = state.lanes.lane_of(event);
            if (is_control(kind))
                continue;
            if (kind == EventKind::PeriodTag && event.name != settings_.timeline)
                continue;
            state.waiting.push_back(FoldEvent{event, lane});
        }
    }

    void PeriodAssembler::place(std::uint32_t thread, const FoldEvent& event)
    {
        const std::uint64_t time_ns = event.event.begin;
        for (OpenPeriod& open : open_)
        {
            if (time_ns < open.period.begin_ns || (open.ended && time_ns >= open.period.end_ns))
                continue;
            if (kind_of(event.event) == EventKind::PeriodTag)
            {
                open.period.tags.push_back(Tag{static_cast<NameID>(event.event.end >> 32U),
                                               static_cast<NameID>(event.event.end)});
            }
            else
            {
                open.events[thread].push_back(event);
            }
            return;
        }
        for (const auto& [begin_ns, end_ns] : closed_)
        {
            if (time_ns >= begin_ns && time_ns < end_ns)
            {
                ++thread_record(thread).late;
                return;
            }
        }
        ++record_.drops.outside_periods;
    }

    void PeriodAssembler::place_waiting(std::uint64_t horizon_ns)
    {
        for (auto& [thread, state] : threads_)
        {
            while (!state.waiting.empty() && state.waiting.front().event.begin <= horizon_ns)
            {
                place(thread, state.waiting.front());
                state.waiting.pop_front();
            }
        }
    }

    void PeriodAssembler::close_ended(std::uint64_t horizon_ns)
    {
        while (!open_.empty() && open_.front().ended && open_.front().period.end_ns <= horizon_ns)
        {
            close(open_.front());
            open_.erase(open_.begin());
        }
    }

    void PeriodAssembler::close(OpenPeriod& open)
    {
        std::vector<ThreadEvents> threads;
        for (auto& [thread, events] : open.events)
            threads.push_back(ThreadEvents{thread, std::move(events)});
        PeriodFold fold = fold_period(threads);
        PeriodRecord& period = open.period;
        period.zones = std::move(fold.zones);
        period.samples = std::move(fold.samples);
        for (const ThreadCount& count : fold.unmatched)
            thread_record(count.thread).unmatched += count.count;
        for (Source* const source : sources_)
        {
            SpanSink sink(period.spans, names_, record_.tracks);
            const PeriodSpan span{period.index, settings_.origin.steady_ns + period.begin_ns,
                                  settings_.origin.steady_ns + period.end_ns};
            source->collect(span, sink);
            record_.drops.spans_dropped += sink.refused() + sink.late();
        }
        copy_tables();
        for (auto writer = writers_.begin(); writer != writers_.end();)
        {
            if (writer->writer->write_period(record_, period, fold.instances) ==
                WriteStatus::Written)
            {
                ++writer;
                continue;
            }
            problems_.push_back("writer \"" + writer->name + "\" failed on period " +
                                std::to_string(period.index) + " and was stopped");
            writer = writers_.erase(writer);
        }
        if (!period.warmup)
            period.runs = std::move(fold.instances);
        closed_.emplace_back(period.begin_ns, period.end_ns);
        record_.periods.push_back(std::move(period));
    }

    ThreadRecord& PeriodAssembler::thread_record(std::uint32_t thread)
    {
        const auto below = [](const ThreadRecord& record, std::uint32_t index)
        {
            return record.index < index;
        };
        auto found =
            std::lower_bound(record_.threads.begin(), record_.threads.end(), thread, below);
        if (found != record_.threads.end() && found->index == thread)
            return *found;
        ThreadRecord added;
        added.index = thread;
        return *record_.threads.insert(found, added);
    }

    void PeriodAssembler::copy_tables()
    {
        record_.names.assign(1U, std::string());
        for (std::size_t id = 1U; id <= names_.size(); ++id)
            record_.names.push_back(names_.text(static_cast<NameID>(id)));
        record_.sites.assign(1U, SiteRecord{});
        for (std::size_t id = 1U; id <= sites_.size(); ++id)
        {
            const SiteInfo info = sites_.info(static_cast<SiteID>(id));
            record_.sites.push_back(SiteRecord{info.name, info.group, info.file, info.unit,
                                               info.line, info.level, info.role, info.op});
        }
    }
} // namespace SushiProfiler
