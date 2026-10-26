/**************************************************************************/
/* period_timeline.cpp                                                    */
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
 * @file period_timeline.cpp
 * @brief Marks periods and tags them, writing each to the calling thread's ring.
 * @author Mustafa Garip
 * @see modules/capture/periods/README.md
 */

#include <SushiProfiler/periods/period_timeline.hpp>

#include <SushiProfiler/collector/collector.hpp>
#include <SushiProfiler/events/trace_event.hpp>
#include <SushiProfiler/rings/thread_ring.hpp>

namespace SushiProfiler
{
    PeriodTimeline::PeriodTimeline(std::string_view name, std::uint64_t budget_ns)
        : name_(name), id_(Collector::instance().names().intern(name)), budget_ns_(budget_ns)
    {

    }

    std::uint64_t PeriodTimeline::mark() noexcept
    {
        return mark(budget_ns_);
    }

    std::uint64_t PeriodTimeline::mark(std::uint64_t budget_ns) noexcept
    {
        const std::uint64_t index = marks_.fetch_add(1U, std::memory_order_relaxed);
        PeriodObserver* const observer = observer_.load(std::memory_order_acquire);
        if (observer != nullptr)
            observer->before_mark(*this, index);
        write_mark(index, budget_ns);
        if (observer != nullptr)
            observer->after_mark(*this, index);
        return index;
    }

    void PeriodTimeline::write_mark(std::uint64_t index, std::uint64_t budget_ns) const noexcept
    {
        if (!Collector::recording())
            return;
        ThreadRing* const ring = Collector::ring();
        if (ring == nullptr)
            return;
        const std::uint64_t now = Collector::now();
        static_cast<void>(
            ring->push(TraceEvent{now, index, pack_site(EventKind::PeriodMark, 0U), id_}));
        if (budget_ns != 0U)
        {
            static_cast<void>(ring->push(
                TraceEvent{now, budget_ns, pack_site(EventKind::PeriodBudget, 0U), id_}));
        }
    }

    void PeriodTimeline::observe(PeriodObserver* observer) noexcept
    {
        observer_.store(observer, std::memory_order_release);
    }

    std::uint64_t PeriodTimeline::current() const noexcept
    {
        const std::uint64_t marks = marks_.load(std::memory_order_relaxed);
        return marks == 0U ? K_NO_PERIOD : marks - 1U;
    }

    void PeriodTimeline::tag(std::string_view key, std::string_view value)
    {
        if (!Collector::recording())
            return;
        NameTable& names = Collector::instance().names();
        const std::uint64_t pair = (std::uint64_t{names.intern(key)} << 32U) | names.intern(value);
        ThreadRing* const ring = Collector::ring();
        if (ring == nullptr)
            return;
        const TraceEvent event{Collector::now(), pair, pack_site(EventKind::PeriodTag, 0U), id_};
        static_cast<void>(ring->push(event));
    }

    NameID PeriodTimeline::id() const noexcept
    {
        return id_;
    }

    std::string_view PeriodTimeline::name() const noexcept
    {
        return name_;
    }
} // namespace SushiProfiler
