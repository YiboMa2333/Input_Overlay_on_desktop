// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <array>
#include <chrono>
#include <cstddef>

namespace overlay {
enum class EventType { Key, Button, Motion, Wheel };
struct InputEvent {
    EventType type;
    unsigned code{};
    bool pressed{};
    long x{}, y{};
    int wheel{};
};

// Owned and updated exclusively by the window message thread.
struct InputState {
    std::array<bool, 256> keys{};
    std::array<bool, 5> buttons{};
    long mouse_x{}, mouse_y{};
    int wheel{};
    std::chrono::steady_clock::time_point wheel_time{};
    void apply(const InputEvent& event);
    void expire();
};
}
