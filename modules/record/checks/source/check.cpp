/**************************************************************************/
/* check.cpp                                                              */
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
 * @file check.cpp
 * @brief Reads checks from JSON, matching each subject kind to the bound keys it takes.
 * @author Mustafa Garip
 * @see modules/record/checks/README.md
 */

#include <SushiProfiler/checks/check.hpp>

#include <array>
#include <cstddef>
#include <utility>

#include <SushiProfiler/json/json_reader.hpp>
#include <SushiProfiler/json/json_value.hpp>

namespace SushiProfiler
{
    namespace
    {
        /// @brief The words of Statistic, in its order.
        constexpr std::array<std::string_view, 7> K_STATISTICS = {"median", "p50", "p90", "p95",
                                                                  "p99",    "min", "max"};

        /// @brief The subject kinds whose samples are times.
        constexpr std::array<std::string_view, 5> K_TIME_KINDS = {"timeline", "group", "zone",
                                                                  "span", "flow"};

        /// @brief The subject kinds whose samples carry their site's unit.
        constexpr std::array<std::string_view, 2> K_PLAIN_KINDS = {"counter", "value"};

        /// @brief Returns whether @p words holds @p word.
        template <std::size_t Count>
        bool holds(const std::array<std::string_view, Count>& words, std::string_view word)
        {
            for (const std::string_view known : words)
            {
                if (known == word)
                    return true;
            }
            return false;
        }

        /// @brief Reads one check at @p path, or returns why not.
        std::string read_check(const JsonValue& value, const std::string& path, Check& out)
        {
            const std::vector<JsonMember>* const members = value.as_object();
            if (members == nullptr)
                return path + ": an object was expected";
            const JsonValue* const subject = value.find("subject");
            if (subject == nullptr || subject->as_string() == nullptr)
                return path + ".subject: a string was expected";
            out.subject = *subject->as_string();
            const std::string kind = out.subject.substr(0U, out.subject.find(':'));
            const bool time = holds(K_TIME_KINDS, kind);
            const bool plain = holds(K_PLAIN_KINDS, kind);
            if (out.subject.find(':') == std::string::npos || (!time && !plain))
            {
                return path + ".subject: \"" + out.subject +
                       "\" does not start with timeline:, group:, zone:, span:, flow:, counter: "
                       "or value:";
            }
            const std::string most = time ? "at_most_ns" : "at_most";
            const std::string least = time ? "at_least_ns" : "at_least";
            for (const JsonMember& member : *members)
            {
                if (member.key == "subject")
                    continue;
                if (member.key == "statistic")
                {
                    const std::string* const word = member.value.as_string();
                    bool known = false;
                    for (std::size_t index = 0U; word != nullptr && index < K_STATISTICS.size();
                         ++index)
                    {
                        if (K_STATISTICS[index] == *word)
                        {
                            out.statistic = static_cast<Statistic>(index);
                            known = true;
                        }
                    }
                    if (!known)
                        return path + ".statistic: not one of median, p50, p90, p95, p99, min, max";
                    continue;
                }
                double bound = 0.0;
                if (member.key != most && member.key != least)
                {
                    return path + ": unknown key \"" + member.key +
                           "\" for this subject; its bounds are " + most + " and " + least;
                }
                if (!member.value.to_double(bound))
                    return path + "." + member.key + ": a number was expected";
                (member.key == most ? out.at_most : out.at_least) = bound;
            }
            if (value.find("statistic") == nullptr)
                return path + ": missing key \"statistic\"";
            if (!out.at_most && !out.at_least)
                return path + ": a check needs " + most + " or " + least;
            return std::string();
        }
    } // namespace

    std::string_view statistic_word(Statistic statistic) noexcept
    {
        return K_STATISTICS[static_cast<std::size_t>(statistic)];
    }

    ChecksReadResult read_checks(std::string_view text)
    {
        ChecksReadResult result;
        const JsonReadResult json = read_json(text);
        if (!json.ok())
        {
            result.error = "line " + std::to_string(json.error.line) + ", column " +
                           std::to_string(json.error.column) + ": " + json.error.message;
            return result;
        }
        const std::vector<JsonValue>* const elements = json.value->as_array();
        if (elements == nullptr)
        {
            result.error = "checks: an array of checks was expected";
            return result;
        }
        for (std::size_t index = 0U; index < elements->size(); ++index)
        {
            Check check;
            result.error =
                read_check((*elements)[index], "checks[" + std::to_string(index) + "]", check);
            if (!result.error.empty())
            {
                result.checks.clear();
                return result;
            }
            result.checks.push_back(std::move(check));
        }
        return result;
    }
} // namespace SushiProfiler
