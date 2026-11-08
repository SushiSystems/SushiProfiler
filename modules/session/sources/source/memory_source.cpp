/**************************************************************************/
/* memory_source.cpp                                                      */
/**************************************************************************/
/*                          This file is part of:                         */
/*                              SushiProfiler                             */
/*              https://github.com/sushisystems/sushiprofiler             */
/*                         https://sushisystems.io                        */
/**************************************************************************/
/* Copyright (c) 2026-present Mustafa Garip & Sushi Systems               */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License"); you    */
/* may not use this file except in compliance with the License. You may   */
/* obtain a copy of the License at                                        */
/*                                                                        */
/*     https://www.apache.org/licenses/LICENSE-2.0                        */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
/**************************************************************************/

/**
 * @file memory_source.cpp
 * @brief Hands the work queued into the in-memory source to the periods it started in.
 * @author Mustafa Garip
 * @see modules/session/sources/README.md
 */

#include <SushiProfiler/sources/memory_source.hpp>

#include <utility>

#include <SushiProfiler/record/span_table.hpp>

namespace SushiProfiler
{
    MemorySource::MemorySource(std::string name, std::uint64_t max_delay_ns)
        : name_(std::move(name)), max_delay_ns_(max_delay_ns)
    {

    }

    void MemorySource::refuse_with(std::string error)
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        refusal_ = std::move(error);
    }

    void MemorySource::queue(MemoryWork work)
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        pending_.push_back(std::move(work));
    }

    void MemorySource::state_fact(std::string name, std::string value)
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        facts_.push_back(StatedFact{std::move(name), std::move(value)});
    }

    std::string_view MemorySource::name() const noexcept
    {
        return name_;
    }

    SourceStatus MemorySource::begin(SourceContext& context)
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        if (!refusal_.empty())
            return SourceStatus{refusal_};
        context_ = &context;
        return SourceStatus{};
    }

    std::uint64_t MemorySource::max_delay_ns() const noexcept
    {
        return max_delay_ns_;
    }

    void MemorySource::collect(const PeriodSpan& period, SpanSink& sink)
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        collected_.push_back(period);
        for (const StatedFact& fact : facts_)
            static_cast<void>(sink.set_fact(fact.name, fact.value));
        std::vector<MemoryWork> later;
        for (MemoryWork& work : pending_)
        {
            if (work.start_ns >= period.end_steady_ns)
            {
                later.push_back(std::move(work));
                continue;
            }
            if (work.start_ns < period.start_steady_ns)
            {
                sink.drop_late(1U);
                continue;
            }
            Span span;
            span.queued_ns = context_->to_record_ns(work.queued_ns);
            span.start_ns = context_->to_record_ns(work.start_ns);
            span.end_ns = context_->to_record_ns(work.end_ns);
            span.track = context_->add_track(work.track);
            span.name = context_->intern(work.name);
            static_cast<void>(sink.append(span));
        }
        pending_ = std::move(later);
    }

    void MemorySource::end()
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        context_ = nullptr;
        ++ends_;
    }

    std::vector<PeriodSpan> MemorySource::collected() const
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        return collected_;
    }

    std::size_t MemorySource::ends() const
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        return ends_;
    }
} // namespace SushiProfiler
