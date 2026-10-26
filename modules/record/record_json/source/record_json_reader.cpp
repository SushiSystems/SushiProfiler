/**************************************************************************/
/* record_json_reader.cpp                                                 */
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
 * @file record_json_reader.cpp
 * @brief Reads a run record from JSON, refusing every key, kind, range and id the format forbids.
 * @author Mustafa Garip
 * @see docs/reference/RECORD_FORMAT.md
 */

#include <SushiProfiler/record_json/record_json.hpp>

#include <algorithm>
#include <limits>
#include <utility>

#include <SushiProfiler/json/json_reader.hpp>

#include "record_decoder.hpp"
#include "record_names.hpp"

namespace SushiProfiler
{
    namespace
    {
        using Detail::RecordDecoder;

        /// @brief The largest value a 32-bit field holds.
        constexpr std::int64_t K_INT32_MAX = std::numeric_limits<std::int32_t>::max();

        /// @brief The largest value a 64-bit signed field holds.
        constexpr std::int64_t K_INT64_MAX = std::numeric_limits<std::int64_t>::max();

        /// @brief The smallest value a 64-bit signed field holds.
        constexpr std::int64_t K_INT64_MIN = std::numeric_limits<std::int64_t>::min();

        /// @brief Reads one record, checking each id against the tables read before it.
        class RecordParser final
        {
            public:
                /// @brief Reads @p root into @p out.
                bool parse(const JsonValue& root, RunRecord& out)
                {
                    if (!decoder_.object(root, {"sushiprofiler_record", "environment", "build",
                                                "tags", "clock", "names", "sites", "tracks",
                                                "threads", "drops", "periods"}))
                        return false;
                    std::uint32_t version = 0U;
                    if (!decoder_.whole(root, "sushiprofiler_record", version))
                        return false;
                    if (version != K_RECORD_FORMAT_VERSION)
                    {
                        const RecordDecoder::Segment segment(decoder_, "sushiprofiler_record");
                        return decoder_.fail("version " + std::to_string(version) +
                                             " is not the version this build reads, " +
                                             std::to_string(K_RECORD_FORMAT_VERSION));
                    }
                    record_ = &out;
                    const auto site = [this](const JsonValue& value) { return this->site(value); };
                    const auto thread = [this](const JsonValue& value)
                    {
                        return this->thread(value);
                    };
                    const auto period = [this](const JsonValue& value)
                    {
                        return this->period(value);
                    };
                    return facts(root, "environment", out.environment) &&
                           facts(root, "build", out.build) && facts(root, "tags", out.tags) &&
                           clock(root) && decoder_.texts(root, "names", out.names) &&
                           decoder_.each(root, "sites", site) &&
                           decoder_.texts(root, "tracks", out.tracks) &&
                           decoder_.each(root, "threads", thread) && drops(root) &&
                           decoder_.each(root, "periods", period);
                }

                /// @brief Returns the first refusal.
                [[nodiscard]] const std::string& error() const noexcept
                {
                    return decoder_.error();
                }

            private:
                /// @brief Reads name and value pairs under @p key into @p out.
                bool facts(const JsonValue& object, std::string_view key, std::vector<Fact>& out)
                {
                    const auto pair = [this, &out](const JsonValue& value)
                    {
                        const std::vector<JsonValue>* const parts = value.as_array();
                        if (parts == nullptr || parts->size() != 2U ||
                            (*parts)[0].as_string() == nullptr ||
                            (*parts)[1].as_string() == nullptr)
                            return decoder_.fail("a pair of strings was expected");
                        out.push_back(Fact{*(*parts)[0].as_string(), *(*parts)[1].as_string()});
                        return true;
                    };
                    return decoder_.each(object, key, pair);
                }

                /// @brief Reads the clock.
                bool clock(const JsonValue& root)
                {
                    const RecordDecoder::Segment segment(decoder_, "clock");
                    const JsonValue& clock = RecordDecoder::member(root, "clock");
                    ClockRecord& out = record_->clock;
                    return decoder_.object(clock,
                                           {"source", "nanoseconds_per_tick", "read_cost_ns"}) &&
                           decoder_.text(clock, "source", out.source) &&
                           decoder_.real(clock, "nanoseconds_per_tick", out.nanoseconds_per_tick) &&
                           decoder_.real(clock, "read_cost_ns", out.read_cost_ns);
                }

                /// @brief Reads one site.
                bool site(const JsonValue& value)
                {
                    SiteRecord site;
                    std::string role;
                    std::string op;
                    if (!decoder_.object(value, {"name", "group", "file", "line", "level", "role",
                                                 "op", "unit"}) ||
                        !decoder_.text(value, "name", site.name) ||
                        !decoder_.text(value, "group", site.group) ||
                        !decoder_.text(value, "file", site.file) ||
                        !decoder_.whole(value, "line", site.line) ||
                        !decoder_.whole(value, "level", site.level) ||
                        !decoder_.text(value, "role", role) || !decoder_.text(value, "op", op) ||
                        !decoder_.text(value, "unit", site.unit))
                        return false;
                    const std::optional<SiteRole> known_role = Detail::role_from(role);
                    if (!known_role)
                        return word_refused("role", role);
                    const std::optional<CounterOp> known_op = Detail::op_from(op);
                    if (!known_op)
                        return word_refused("op", op);
                    site.role = *known_role;
                    site.op = *known_op;
                    record_->sites.push_back(std::move(site));
                    return true;
                }

                /// @brief Refuses @p word, found under @p key.
                bool word_refused(std::string_view key, const std::string& word)
                {
                    const RecordDecoder::Segment segment(decoder_, key);
                    return decoder_.fail("\"" + word + "\" is not a known word");
                }

                /// @brief Reads one thread.
                bool thread(const JsonValue& value)
                {
                    ThreadRecord thread;
                    if (!decoder_.object(value, {"index", "name", "dropped", "late"}) ||
                        !decoder_.whole(value, "index", thread.index) ||
                        !decoder_.text(value, "name", thread.name) ||
                        !decoder_.whole(value, "dropped", thread.dropped) ||
                        !decoder_.whole(value, "late", thread.late))
                        return false;
                    if (std::binary_search(thread_ids_.begin(), thread_ids_.end(), thread.index))
                    {
                        const RecordDecoder::Segment segment(decoder_, "index");
                        return decoder_.fail("thread " + std::to_string(thread.index) +
                                             " appears twice");
                    }
                    thread_ids_.insert(std::upper_bound(thread_ids_.begin(), thread_ids_.end(),
                                                        thread.index),
                                       thread.index);
                    record_->threads.push_back(std::move(thread));
                    return true;
                }

                /// @brief Refuses thread index @p thread under @p key unless the table holds it.
                bool known_thread(std::string_view key, std::uint32_t thread)
                {
                    if (std::binary_search(thread_ids_.begin(), thread_ids_.end(), thread))
                        return true;
                    const RecordDecoder::Segment segment(decoder_, key);
                    return decoder_.fail("thread " + std::to_string(thread) +
                                         " is not in the thread table");
                }

                /// @brief Reads the drops.
                bool drops(const JsonValue& root)
                {
                    const RecordDecoder::Segment segment(decoder_, "drops");
                    const JsonValue& drops = RecordDecoder::member(root, "drops");
                    RecordDrops& out = record_->drops;
                    return decoder_.object(drops, {"outside_periods", "open_at_stop",
                                                   "spans_dropped"}) &&
                           decoder_.whole(drops, "outside_periods", out.outside_periods) &&
                           decoder_.whole(drops, "open_at_stop", out.open_at_stop) &&
                           decoder_.whole(drops, "spans_dropped", out.spans_dropped);
                }

                /// @brief Reads one period.
                bool period(const JsonValue& value)
                {
                    PeriodRecord period;
                    const auto tree = [this, &period](const JsonValue& entry)
                    {
                        return zone_tree(entry, period.zones);
                    };
                    const auto series = [this, &period](const JsonValue& entry)
                    {
                        return samples(entry, period.samples);
                    };
                    if (!decoder_.object(value, {"index", "timeline", "thread", "begin_ns",
                                                 "end_ns", "warmup", "budget_ns", "tags", "facts",
                                                 "zones", "spans", "samples"}) ||
                        !decoder_.whole(value, "index", period.index) ||
                        !decoder_.whole(value, "timeline", period.timeline) ||
                        !decoder_.below("timeline", period.timeline, names(), "names") ||
                        !decoder_.whole(value, "thread", period.thread) ||
                        !known_thread("thread", period.thread) ||
                        !decoder_.whole(value, "begin_ns", period.begin_ns) ||
                        !decoder_.whole(value, "end_ns", period.end_ns) ||
                        !decoder_.flag(value, "warmup", period.warmup) ||
                        !decoder_.whole(value, "budget_ns", period.budget_ns) ||
                        !tags(value, period.tags) || !facts(value, "facts", period.facts) ||
                        !decoder_.each(value, "zones", tree) || !spans(value, period.spans) ||
                        !decoder_.each(value, "samples", series))
                        return false;
                    if (period.end_ns < period.begin_ns)
                    {
                        const RecordDecoder::Segment segment(decoder_, "end_ns");
                        return decoder_.fail("ends before it begins");
                    }
                    record_->periods.push_back(std::move(period));
                    return true;
                }

                /// @brief Reads a period's tags as key and value name pairs.
                bool tags(const JsonValue& period, std::vector<Tag>& out)
                {
                    const auto pair = [this, &out](const JsonValue& entry)
                    {
                        const std::vector<JsonValue>* const parts = entry.as_array();
                        std::uint64_t key = 0U;
                        std::uint64_t value = 0U;
                        if (parts == nullptr || parts->size() != 2U ||
                            !(*parts)[0].to_uint64(key) || !(*parts)[1].to_uint64(value))
                            return decoder_.fail("a pair of name ids was expected");
                        if (key >= names() || value >= names())
                            return decoder_.fail("points past the names table");
                        out.push_back(Tag{static_cast<NameID>(key), static_cast<NameID>(value)});
                        return true;
                    };
                    return decoder_.each(period, "tags", pair);
                }

                /// @brief Reads one zone tree.
                bool zone_tree(const JsonValue& value, std::vector<ZoneTree>& out)
                {
                    ZoneTree tree;
                    std::vector<SiteID> site;
                    std::vector<NameID> name;
                    std::vector<std::int64_t> parent;
                    std::vector<std::uint64_t> inclusive;
                    std::vector<std::uint64_t> self;
                    std::vector<std::uint64_t> calls;
                    if (!decoder_.object(value, {"thread", "lane", "site", "name", "parent",
                                                 "inclusive_ns", "self_ns", "calls"}) ||
                        !decoder_.whole(value, "thread", tree.thread) ||
                        !known_thread("thread", tree.thread) ||
                        !decoder_.whole(value, "lane", tree.lane) ||
                        !decoder_.below("lane", tree.lane, names(), "names") ||
                        !decoder_.wholes(value, "site", site) ||
                        !decoder_.wholes(value, "name", name) ||
                        !decoder_.integers(value, "parent", -1, K_INT32_MAX, parent) ||
                        !decoder_.wholes(value, "inclusive_ns", inclusive) ||
                        !decoder_.wholes(value, "self_ns", self) ||
                        !decoder_.wholes(value, "calls", calls))
                        return false;
                    const std::size_t count = site.size();
                    if (!decoder_.length("name", name.size(), count) ||
                        !decoder_.length("parent", parent.size(), count) ||
                        !decoder_.length("inclusive_ns", inclusive.size(), count) ||
                        !decoder_.length("self_ns", self.size(), count) ||
                        !decoder_.length("calls", calls.size(), count) ||
                        !decoder_.all_below("site", site, sites(), "sites") ||
                        !decoder_.all_below("name", name, names(), "names"))
                        return false;
                    for (std::size_t index = 0U; index < count; ++index)
                    {
                        if (parent[index] >= static_cast<std::int64_t>(index))
                        {
                            const RecordDecoder::Segment segment(decoder_, "parent");
                            const RecordDecoder::Segment at(decoder_, index);
                            return decoder_.fail("points at no earlier node");
                        }
                        tree.nodes.push_back(ZoneNode{site[index], name[index],
                                                      static_cast<std::int32_t>(parent[index]),
                                                      inclusive[index], self[index], calls[index]});
                    }
                    out.push_back(std::move(tree));
                    return true;
                }

                /// @brief Reads a period's span columns and attribute columns.
                bool spans(const JsonValue& period, SpanTable& out)
                {
                    const RecordDecoder::Segment segment(decoder_, "spans");
                    const JsonValue& value = RecordDecoder::member(period, "spans");
                    std::vector<std::uint64_t> queued;
                    std::vector<std::uint64_t> start;
                    std::vector<std::uint64_t> end;
                    std::vector<TrackID> track;
                    std::vector<NameID> name;
                    std::vector<std::uint32_t> submit_thread;
                    std::vector<std::uint32_t> uncertainty;
                    if (!decoder_.object(value, {"queued_ns", "start_ns", "end_ns", "track",
                                                 "name", "submit_thread", "uncertainty_ns",
                                                 "attributes"}) ||
                        !decoder_.wholes(value, "queued_ns", queued) ||
                        !decoder_.wholes(value, "start_ns", start) ||
                        !decoder_.wholes(value, "end_ns", end) ||
                        !decoder_.wholes(value, "track", track) ||
                        !decoder_.wholes(value, "name", name) ||
                        !decoder_.wholes(value, "submit_thread", submit_thread) ||
                        !decoder_.wholes(value, "uncertainty_ns", uncertainty))
                        return false;
                    const std::size_t count = queued.size();
                    if (!decoder_.length("start_ns", start.size(), count) ||
                        !decoder_.length("end_ns", end.size(), count) ||
                        !decoder_.length("track", track.size(), count) ||
                        !decoder_.length("name", name.size(), count) ||
                        !decoder_.length("submit_thread", submit_thread.size(), count) ||
                        !decoder_.length("uncertainty_ns", uncertainty.size(), count) ||
                        !decoder_.all_below("track", track, record_->tracks.size(), "tracks") ||
                        !decoder_.all_below("name", name, names(), "names"))
                        return false;
                    for (std::size_t row = 0U; row < count; ++row)
                    {
                        if (!known_thread("submit_thread", submit_thread[row]))
                            return false;
                        out.append(Span{queued[row], start[row], end[row], track[row], name[row],
                                        submit_thread[row], uncertainty[row]});
                    }
                    const auto column = [this, &out](const JsonValue& entry)
                    {
                        return attribute(entry, out);
                    };
                    return decoder_.each(value, "attributes", column);
                }

                /// @brief Reads one attribute column of @p table.
                bool attribute(const JsonValue& value, SpanTable& table)
                {
                    AttributeColumn column;
                    std::string kind;
                    if (!decoder_.object(value, {"key", "kind", "values", "present"}) ||
                        !decoder_.whole(value, "key", column.key) ||
                        !decoder_.below("key", column.key, names(), "names") ||
                        !decoder_.text(value, "kind", kind))
                        return false;
                    const std::optional<AttributeKind> known = Detail::kind_from(kind);
                    if (!known)
                        return word_refused("kind", kind);
                    column.kind = *known;
                    const std::size_t count = table.size();
                    if (column.kind == AttributeKind::Real)
                    {
                        if (!decoder_.reals(value, "values", column.reals))
                            return false;
                        column.integers.assign(column.reals.size(), 0);
                    }
                    else
                    {
                        if (!decoder_.integers(value, "values", K_INT64_MIN, K_INT64_MAX,
                                               column.integers))
                            return false;
                        if (column.kind == AttributeKind::Name &&
                            !decoder_.all_below("values", column.integers, names(), "names"))
                            return false;
                        column.reals.assign(column.integers.size(), 0.0);
                    }
                    if (!decoder_.wholes(value, "present", column.present) ||
                        !decoder_.length("values", column.integers.size(), count) ||
                        !decoder_.length("present", column.present.size(), count))
                        return false;
                    if (!decoder_.all_below("present", column.present, 2U, "0 and 1"))
                        return false;
                    if (table.attribute(column.key) != nullptr)
                    {
                        const RecordDecoder::Segment segment(decoder_, "key");
                        return decoder_.fail("the attribute appears twice");
                    }
                    table.add_column(std::move(column));
                    return true;
                }

                /// @brief Reads one sample series.
                bool samples(const JsonValue& value, std::vector<SampleSeries>& out)
                {
                    SampleSeries series;
                    std::vector<std::string> points;
                    if (!decoder_.object(value, {"site", "thread", "time_ns", "values", "items",
                                                 "points"}) ||
                        !decoder_.whole(value, "site", series.site) ||
                        !decoder_.below("site", series.site, sites(), "sites") ||
                        !decoder_.whole(value, "thread", series.thread) ||
                        !known_thread("thread", series.thread) ||
                        !decoder_.wholes(value, "time_ns", series.time_ns) ||
                        !decoder_.reals(value, "values", series.values) ||
                        !decoder_.wholes(value, "items", series.items) ||
                        !decoder_.texts(value, "points", points))
                        return false;
                    const std::size_t count = series.time_ns.size();
                    const std::size_t values = series.values.size();
                    const std::size_t items = series.items.size();
                    if ((values != 0U && !decoder_.length("values", values, count)) ||
                        (items != 0U && !decoder_.length("items", items, count)) ||
                        !decoder_.length("points", points.size(), items))
                        return false;
                    for (std::size_t index = 0U; index < points.size(); ++index)
                    {
                        const std::optional<FlowPoint> point = Detail::point_from(points[index]);
                        if (!point)
                        {
                            const RecordDecoder::Segment segment(decoder_, "points");
                            const RecordDecoder::Segment at(decoder_, index);
                            return decoder_.fail("\"" + points[index] + "\" is not a known word");
                        }
                        series.points.push_back(*point);
                    }
                    out.push_back(std::move(series));
                    return true;
                }

                /// @brief Returns the size of the names table.
                [[nodiscard]] std::size_t names() const noexcept
                {
                    return record_->names.size();
                }

                /// @brief Returns the size of the sites table.
                [[nodiscard]] std::size_t sites() const noexcept
                {
                    return record_->sites.size();
                }

                /// @brief The field reader.
                RecordDecoder decoder_;

                /// @brief The record being filled.
                RunRecord* record_ = nullptr;

                /// @brief The thread indices read so far, sorted.
                std::vector<std::uint32_t> thread_ids_;
        };
    } // namespace

    RecordReadResult read_record(std::string_view text)
    {
        RecordReadResult result;
        const JsonReadResult json = read_json(text);
        if (!json.ok())
        {
            result.error = "line " + std::to_string(json.error.line) + ", column " +
                           std::to_string(json.error.column) + ": " + json.error.message;
            return result;
        }
        RunRecord record;
        RecordParser parser;
        if (parser.parse(*json.value, record))
            result.record = std::move(record);
        else
            result.error = parser.error();
        return result;
    }
} // namespace SushiProfiler
