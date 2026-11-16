// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <unordered_map>

namespace overlay {
enum class EventType { Key, Button, Motion, Wheel };
// Set-1 physical scan codes, independent of the active keyboard layout.
enum Key : unsigned { Q = 0x10, W = 0x11, E = 0x12, A = 0x1e,
    S = 0x1f, D = 0x20, LeftShift = 0x2a, RightShift = 0x36,
    LeftCtrl = 0x1d, RightCtrl = 0x9d, Space = 0x39 };
inline constexpr std::array<unsigned, 11> tracked_keys{
    W, A, S, D, Q, E, LeftShift, RightShift, LeftCtrl, RightCtrl, Space};
constexpr bool tracked(unsigned code) {
    for (auto key : tracked_keys) if (key == code) return true;
    return false;
}
struct InputEvent {
    EventType type;
    unsigned code{};
    bool pressed{};
    long x{}, y{};
    int wheel{};
};

// Owned and updated exclusively by the window message thread.
struct InputState {
    using Clock = std::chrono::steady_clock;
    using Device = std::uintptr_t;
    std::array<bool, 256> keys{};
    std::array<bool, 5> buttons{};
    long mouse_x{}, mouse_y{};
    int wheel{};
    std::chrono::steady_clock::time_point wheel_time{};
    void apply(const InputEvent& event, Device device = 0, Clock::time_point now = Clock::now());
    void expire(Clock::time_point now = Clock::now());
    void remove_device(Device device);
    void clear();
    // Release-only safety net. Never invents presses from asynchronous polling.
    void reconcile(const std::array<bool, 256>& physical_keys,
        const std::array<bool, 5>& physical_buttons, Clock::time_point now = Clock::now());
private:
    struct DeviceState {
        std::array<bool, 256> keys{};
        std::array<bool, 5> buttons{};
        std::array<Clock::time_point, 256> key_time{};
        std::array<Clock::time_point, 5> button_time{};
    };
    std::unordered_map<Device, DeviceState> devices_;
    void aggregate();
};
}
