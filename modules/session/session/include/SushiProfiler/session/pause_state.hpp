/**************************************************************************/
/* pause_state.hpp                                                        */
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
 * @file pause_state.hpp
 * @brief Declares the value that decides whether a session pause or resume takes effect.
 * @author Mustafa Garip
 * @see modules/session/session/README.md
 */

#pragma once

namespace SushiProfiler
{
    /**
     * @brief Holds whether a session is paused and says whether pausing or resuming took effect.
     *
     * A pause takes effect while the session runs and is not paused; a resume takes effect while
     * it runs and is paused.
     */
    class PauseState
    {
        public:
            /// @brief Pauses when @p running and not paused yet; returns whether it did.
            [[nodiscard]] bool pause(bool running) noexcept
            {
                if (!running || paused_)
                    return false;
                paused_ = true;
                return true;
            }

            /// @brief Resumes when @p running and paused; returns whether it did.
            [[nodiscard]] bool resume(bool running) noexcept
            {
                if (!running || !paused_)
                    return false;
                paused_ = false;
                return true;
            }

            /// @brief Returns whether a pause is in effect.
            [[nodiscard]] bool paused() const noexcept
            {
                return paused_;
            }

            /// @brief Forgets a pause, as a run ends or a new one begins.
            void clear() noexcept
            {
                paused_ = false;
            }

        private:
            /// @brief Whether a pause is in effect.
            bool paused_ = false;
    };
} // namespace SushiProfiler
