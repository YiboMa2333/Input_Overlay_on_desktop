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
// Further modifications (2026): per-device holds, physical-key allowlist,
// deterministic timestamps, device/reset cleanup and release-only recovery.
#include "input_state.hpp"

namespace overlay {
void InputState::apply(const InputEvent& event, Device device, Clock::time_point now) {
    if (event.type == EventType::Wheel) {
        wheel = event.wheel;
        wheel_time = now;
    } else if (event.type == EventType::Motion) {
        mouse_x = event.x;
        mouse_y = event.y;
    } else if (event.type == EventType::Key && tracked(event.code)) {
        auto& source = devices_[device];
        source.keys[event.code] = event.pressed;
        source.key_time[event.code] = now;
        aggregate();
    } else if (event.type == EventType::Button && event.code < buttons.size()) {
        auto& source = devices_[device];
        source.buttons[event.code] = event.pressed;
        source.button_time[event.code] = now;
        aggregate();
    }
}
void InputState::expire(Clock::time_point now) {
    if (now - wheel_time >= std::chrono::milliseconds(180)) wheel = 0;
}
void InputState::aggregate() {
    keys.fill(false); buttons.fill(false);
    for (const auto& [device, source] : devices_) {
        (void)device;
        for (auto key : tracked_keys) keys[key] = keys[key] || source.keys[key];
        for (std::size_t i = 0; i < buttons.size(); ++i) buttons[i] = buttons[i] || source.buttons[i];
    }
}
void InputState::remove_device(Device device) {
    devices_.erase(device); aggregate();
    wheel = 0; // A wheel pulse has no persistent owner after removal.
}
void InputState::clear() {
    devices_.clear(); keys.fill(false); buttons.fill(false);
    wheel = 0; wheel_time = {}; mouse_x = mouse_y = 0;
}
void InputState::reconcile(const std::array<bool, 256>& physical_keys,
    const std::array<bool, 5>& physical_buttons, Clock::time_point now) {
    constexpr auto grace = std::chrono::milliseconds(100);
    for (auto& [device, source] : devices_) {
        (void)device;
        for (auto key : tracked_keys)
            if (!physical_keys[key] && now - source.key_time[key] >= grace) source.keys[key] = false;
        for (std::size_t i = 0; i < buttons.size(); ++i)
            if (!physical_buttons[i] && now - source.button_time[i] >= grace) source.buttons[i] = false;
    }
    aggregate();
}
}
