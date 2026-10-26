/**************************************************************************/
/* period_observer.hpp                                                    */
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
 * @file period_observer.hpp
 * @brief Declares the interface a timeline calls around each mark.
 * @author Mustafa Garip
 * @see modules/capture/periods/README.md
 */

#pragma once

#include <cstdint>

namespace SushiProfiler
{
    class PeriodTimeline;

    /**
     * @brief Hears each mark of the timeline it observes, before and after the mark is written.
     *
     * A capture window implements it: it opens the gate before a recorded period's mark and closes
     * it after the mark of the first period it does not record, so that period's end is written.
     * Both calls run on the marking thread.
     */
    class PeriodObserver
    {
        public:
            virtual ~PeriodObserver() = default;

            /// @brief Runs before the mark that opens period @p index is written.
            virtual void before_mark(const PeriodTimeline& timeline,
                                     std::uint64_t index) noexcept = 0;

            /// @brief Runs after the mark that opens period @p index is written, or skipped.
            virtual void after_mark(const PeriodTimeline& timeline,
                                    std::uint64_t index) noexcept = 0;

        protected:
            PeriodObserver() = default;
            PeriodObserver(const PeriodObserver&) = default;
            PeriodObserver& operator=(const PeriodObserver&) = default;
            PeriodObserver(PeriodObserver&&) = default;
            PeriodObserver& operator=(PeriodObserver&&) = default;
    };
} // namespace SushiProfiler
