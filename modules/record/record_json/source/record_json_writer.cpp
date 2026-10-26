/**************************************************************************/
/* record_json_writer.cpp                                                 */
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
 * @file record_json_writer.cpp
 * @brief Writes a run record as indented JSON, every column on one line.
 * @author Mustafa Garip
 * @see docs/reference/RECORD_FORMAT.md
 */

#include <SushiProfiler/record_json/record_json.hpp>

#include <SushiProfiler/json/json_writer.hpp>

#include "record_names.hpp"

#include <cmath>
#include <vector>

namespace SushiProfiler
{
    namespace
    {
        /// @brief Writes one record through one JSON writer.
        class RecordEncoder final
        {
            public:
                /// @brief Builds an encoder that writes indented JSON.
                RecordEncoder() : out_(true)
                {

                }

                /// @brief Writes @p record and returns the text.
                std::string encode(const RunRecord& record)
                {
                    out_.begin_object();
                    out_.key("sushiprofiler_record");
                    out_.unsigned_integer(K_RECORD_FORMAT_VERSION);
                    facts("environment", record.environment);
                    facts("build", record.build);
                    facts("tags", record.tags);
                    clock(record.clock);
                    out_.key("names");
                    strings(record.names);
                    sites(record.sites);
                    out_.key("tracks");
                    strings(record.tracks);
                    threads(record.threads);
                    drops(record.drops);
                    out_.key("periods");
                    out_.begin_array();
                    for (const PeriodRecord& period : record.periods)
                        this->period(period);
                    out_.end_array();
                    out_.end_object();
                    std::string text = out_.take();
                    text.push_back('\n');
                    return text;
                }

            private:
                /// @brief Writes @p value, a non-finite one as the string the format names.
                void real(double value)
                {
                    if (std::isnan(value))
                        out_.string("NaN");
                    else if (std::isinf(value))
                        out_.string(value > 0.0 ? "Infinity" : "-Infinity");
                    else
                        out_.real(value);
                }

                /// @brief Writes @p values as one line of strings.
                void strings(const std::vector<std::string>& values)
                {
                    out_.begin_array(JsonLayout::Inline);
                    for (const std::string& value : values)
                        out_.string(value);
                    out_.end_array();
                }

                /// @brief Writes column @p name of unsigned whole numbers.
                template <class Whole>
                void column(const char* name, const std::vector<Whole>& values)
                {
                    out_.key(name);
                    out_.begin_array(JsonLayout::Inline);
                    for (const Whole value : values)
                        out_.unsigned_integer(value);
                    out_.end_array();
                }

                /// @brief Writes column @p name of reals.
                void real_column(const char* name, const std::vector<double>& values)
                {
                    out_.key(name);
                    out_.begin_array(JsonLayout::Inline);
                    for (const double value : values)
                        real(value);
                    out_.end_array();
                }

                /// @brief Writes @p facts under @p name as name and value pairs.
                void facts(const char* name, const std::vector<Fact>& facts)
                {
                    out_.key(name);
                    out_.begin_array();
                    for (const Fact& fact : facts)
                    {
                        out_.begin_array(JsonLayout::Inline);
                        out_.string(fact.name);
                        out_.string(fact.value);
                        out_.end_array();
                    }
                    out_.end_array();
                }

                /// @brief Writes the clock.
                void clock(const ClockRecord& clock)
                {
                    out_.key("clock");
                    out_.begin_object(JsonLayout::Inline);
                    out_.key("source");
                    out_.string(clock.source);
                    out_.key("nanoseconds_per_tick");
                    real(clock.nanoseconds_per_tick);
                    out_.key("read_cost_ns");
                    real(clock.read_cost_ns);
                    out_.end_object();
                }

                /// @brief Writes the site table, one site per line.
                void sites(const std::vector<SiteRecord>& sites)
                {
                    out_.key("sites");
                    out_.begin_array();
                    for (const SiteRecord& site : sites)
                    {
                        out_.begin_object(JsonLayout::Inline);
                        out_.key("name");
                        out_.string(site.name);
                        out_.key("group");
                        out_.string(site.group);
                        out_.key("file");
                        out_.string(site.file);
                        out_.key("line");
                        out_.unsigned_integer(site.line);
                        out_.key("level");
                        out_.unsigned_integer(site.level);
                        out_.key("role");
                        out_.string(Detail::role_word(site.role));
                        out_.key("op");
                        out_.string(Detail::op_word(site.op));
                        out_.key("unit");
                        out_.string(site.unit);
                        out_.end_object();
                    }
                    out_.end_array();
                }

                /// @brief Writes the thread table, one thread per line.
                void threads(const std::vector<ThreadRecord>& threads)
                {
                    out_.key("threads");
                    out_.begin_array();
                    for (const ThreadRecord& thread : threads)
                    {
                        out_.begin_object(JsonLayout::Inline);
                        out_.key("index");
                        out_.unsigned_integer(thread.index);
                        out_.key("name");
                        out_.string(thread.name);
                        out_.key("dropped");
                        out_.unsigned_integer(thread.dropped);
                        out_.key("late");
                        out_.unsigned_integer(thread.late);
                        out_.end_object();
                    }
                    out_.end_array();
                }

                /// @brief Writes what the run lost outside any thread.
                void drops(const RecordDrops& drops)
                {
                    out_.key("drops");
                    out_.begin_object(JsonLayout::Inline);
                    out_.key("outside_periods");
                    out_.unsigned_integer(drops.outside_periods);
                    out_.key("open_at_stop");
                    out_.unsigned_integer(drops.open_at_stop);
                    out_.key("spans_dropped");
                    out_.unsigned_integer(drops.spans_dropped);
                    out_.end_object();
                }

                /// @brief Writes one period.
                void period(const PeriodRecord& period)
                {
                    out_.begin_object();
                    out_.key("index");
                    out_.unsigned_integer(period.index);
                    out_.key("timeline");
                    out_.unsigned_integer(period.timeline);
                    out_.key("thread");
                    out_.unsigned_integer(period.thread);
                    out_.key("begin_ns");
                    out_.unsigned_integer(period.begin_ns);
                    out_.key("end_ns");
                    out_.unsigned_integer(period.end_ns);
                    out_.key("warmup");
                    out_.boolean(period.warmup);
                    out_.key("budget_ns");
                    out_.unsigned_integer(period.budget_ns);
                    out_.key("tags");
                    out_.begin_array(JsonLayout::Inline);
                    for (const Tag& tag : period.tags)
                    {
                        out_.begin_array(JsonLayout::Inline);
                        out_.unsigned_integer(tag.key);
                        out_.unsigned_integer(tag.value);
                        out_.end_array();
                    }
                    out_.end_array();
                    facts("facts", period.facts);
                    out_.key("zones");
                    out_.begin_array();
                    for (const ZoneTree& tree : period.zones)
                        zone_tree(tree);
                    out_.end_array();
                    spans(period.spans);
                    out_.key("samples");
                    out_.begin_array();
                    for (const SampleSeries& series : period.samples)
                        samples(series);
                    out_.end_array();
                    out_.end_object();
                }

                /// @brief Writes one zone tree as columns.
                void zone_tree(const ZoneTree& tree)
                {
                    std::vector<std::uint64_t> site;
                    std::vector<std::uint64_t> name;
                    std::vector<std::uint64_t> inclusive;
                    std::vector<std::uint64_t> self;
                    std::vector<std::uint64_t> calls;
                    for (const ZoneNode& node : tree.nodes)
                    {
                        site.push_back(node.site);
                        name.push_back(node.name);
                        inclusive.push_back(node.inclusive_ns);
                        self.push_back(node.self_ns);
                        calls.push_back(node.calls);
                    }
                    out_.begin_object();
                    out_.key("thread");
                    out_.unsigned_integer(tree.thread);
                    out_.key("lane");
                    out_.unsigned_integer(tree.lane);
                    column("site", site);
                    column("name", name);
                    out_.key("parent");
                    out_.begin_array(JsonLayout::Inline);
                    for (const ZoneNode& node : tree.nodes)
                        out_.integer(node.parent);
                    out_.end_array();
                    column("inclusive_ns", inclusive);
                    column("self_ns", self);
                    column("calls", calls);
                    out_.end_object();
                }

                /// @brief Writes a period's spans as columns, then their attribute columns.
                void spans(const SpanTable& table)
                {
                    std::vector<std::uint64_t> queued;
                    std::vector<std::uint64_t> start;
                    std::vector<std::uint64_t> end;
                    std::vector<std::uint64_t> track;
                    std::vector<std::uint64_t> name;
                    std::vector<std::uint64_t> submit_thread;
                    std::vector<std::uint64_t> uncertainty;
                    for (std::size_t row = 0U; row < table.size(); ++row)
                    {
                        const Span span = table.span(row);
                        queued.push_back(span.queued_ns);
                        start.push_back(span.start_ns);
                        end.push_back(span.end_ns);
                        track.push_back(span.track);
                        name.push_back(span.name);
                        submit_thread.push_back(span.submit_thread);
                        uncertainty.push_back(span.uncertainty_ns);
                    }
                    out_.key("spans");
                    out_.begin_object();
                    column("queued_ns", queued);
                    column("start_ns", start);
                    column("end_ns", end);
                    column("track", track);
                    column("name", name);
                    column("submit_thread", submit_thread);
                    column("uncertainty_ns", uncertainty);
                    out_.key("attributes");
                    out_.begin_array();
                    for (const AttributeColumn& attribute : table.attributes())
                        this->attribute(attribute);
                    out_.end_array();
                    out_.end_object();
                }

                /// @brief Writes one attribute column.
                void attribute(const AttributeColumn& attribute)
                {
                    out_.begin_object();
                    out_.key("key");
                    out_.unsigned_integer(attribute.key);
                    out_.key("kind");
                    out_.string(Detail::kind_word(attribute.kind));
                    if (attribute.kind == AttributeKind::Real)
                    {
                        real_column("values", attribute.reals);
                    }
                    else
                    {
                        out_.key("values");
                        out_.begin_array(JsonLayout::Inline);
                        for (const std::int64_t value : attribute.integers)
                            out_.integer(value);
                        out_.end_array();
                    }
                    column("present", attribute.present);
                    out_.end_object();
                }

                /// @brief Writes one sample series.
                void samples(const SampleSeries& series)
                {
                    out_.begin_object();
                    out_.key("site");
                    out_.unsigned_integer(series.site);
                    out_.key("thread");
                    out_.unsigned_integer(series.thread);
                    column("time_ns", series.time_ns);
                    real_column("values", series.values);
                    column("items", series.items);
                    out_.key("points");
                    out_.begin_array(JsonLayout::Inline);
                    for (const FlowPoint point : series.points)
                        out_.string(Detail::point_word(point));
                    out_.end_array();
                    out_.end_object();
                }

                /// @brief The writer.
                JsonWriter out_;
        };
    } // namespace

    std::string write_record(const RunRecord& record)
    {
        return RecordEncoder().encode(record);
    }
} // namespace SushiProfiler
