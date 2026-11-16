/*************************************************************************
 * This file is part of input-overlay
 * git.vrsal.xyz/alex/input-overlay
 * Copyright 2023 Alex <uni@vrsal.xyz>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *************************************************************************/

// SPDX-License-Identifier: GPL-2.0-only
// Adapted from deps/common/input_data.cpp in Input Overlay.
// Modifications (2026): replace uiohook events, OBS clock and SDL data with
// a standalone event type, bounded arrays and std::chrono; omit networking/gamepads.
#include "input_state.hpp"

namespace overlay {
void InputState::apply(const InputEvent& event) {
    if (event.type == EventType::Wheel) {
        wheel = event.wheel;
        wheel_time = std::chrono::steady_clock::now();
    } else if (event.type == EventType::Motion) {
        mouse_x = event.x;
        mouse_y = event.y;
    } else if (event.type == EventType::Key && event.code < keys.size()) {
        keys[event.code] = event.pressed;
    } else if (event.type == EventType::Button && event.code < buttons.size()) {
        buttons[event.code] = event.pressed;
    }
}
void InputState::expire() {
    if (std::chrono::steady_clock::now() - wheel_time > std::chrono::milliseconds(180)) wheel = 0;
}
}
