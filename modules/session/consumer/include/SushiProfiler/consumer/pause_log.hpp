/**************************************************************************/
/* pause_log.hpp                                                          */
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
 * @file pause_log.hpp
 * @brief Declares the log of the intervals a session was paused, on the steady clock.
 * @author Mustafa Garip
 * @see modules/session/consumer/README.md
 */

#pragma once

#include <SushiProfiler/version/export.hpp>

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiProfiler
{
    /// @brief The fact a period carries when it overlaps a pause; its value is `true`.
    inline constexpr const char* K_PAUSED_FACT = "paused";

    /**
     * @brief Records when the session was paused, so the consumer can mark the periods it crossed.
     *
     * The session calls begin() before it holds the collector's gate and end() before it releases
     * it, so the log is never behind the gate. Any thread may call any member.
     */
    class SUSHIPROFILER_API PauseLog final
    {
        public:
            /// @brief The most closed intervals kept; the oldest is dropped past it.
            static constexpr std::size_t K_MAX_INTERVALS = 4096U;

            /// @brief Opens an interval at @p steady_ns; ignored while one is open.
            void begin(std::uint64_t steady_ns);

            /// @brief Closes the open interval at @p steady_ns; ignored with none open.
            void end(std::uint64_t steady_ns);

            /// @brief Returns whether an interval is open.
            [[nodiscard]] bool paused() const;

            /// @brief Returns whether [@p begin_steady_ns, @p end_steady_ns) overlaps an interval.
            [[nodiscard]] bool overlaps(std::uint64_t begin_steady_ns,
                                        std::uint64_t end_steady_ns) const;

            /// @brief Returns the end of every closed interval later than @p steady_ns.
            [[nodiscard]] std::vector<std::uint64_t> ends_after(std::uint64_t steady_ns) const;

            /// @brief Drops the closed intervals that ended before @p steady_ns.
            void forget_before(std::uint64_t steady_ns);

        private:
            /// @brief One closed interval.
            struct Interval
            {
                std::uint64_t begin_ns; /**< When the pause began. */
                std::uint64_t end_ns;   /**< When it ended. */
            };

            mutable std::mutex mutex_;          /**< Guards every member below. */
            std::vector<Interval> closed_;      /**< The closed intervals, oldest first. */
            bool open_ = false;                 /**< Whether an interval is open. */
            std::uint64_t open_begin_ns_ = 0U;  /**< When the open interval began. */
    };
} // namespace SushiProfiler
SUSHIPROFILER_DLL_WARNINGS_POP
