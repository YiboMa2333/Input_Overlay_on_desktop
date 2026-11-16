// SPDX-License-Identifier: GPL-2.0-only
#include "input/input_state.hpp"
#include <cstdlib>
#include <iostream>
using namespace overlay;
using namespace std::chrono_literals;
namespace {
void check(bool condition, const char* description) {
    if (!condition) { std::cerr << "FAIL: " << description << '\n'; std::exit(1); }
}
}
int main() {
    InputState state;
    const auto start = InputState::Clock::time_point{} + 1s;
    auto key = [&](unsigned code, bool down, InputState::Device device = 1) {
        state.apply({EventType::Key, code, down}, device, start);
    };
    check(!state.keys[W] && !state.buttons[0], "starts released");
    key(W, true); key(D, true);
    check(state.keys[W] && state.keys[D], "W+D chord");
    key(W, false);
    check(!state.keys[W] && state.keys[D], "W release preserves D");
    key(D, true); key(D, true); key(D, false);
    check(!state.keys[D], "autorepeat is idempotent");
    for (auto code : tracked_keys) key(code, true);
    for (auto code : tracked_keys) check(state.keys[code], "all requested physical keys tracked");
    state.clear();
    key(0xff, true); key(500, true);
    for (bool held : state.keys) check(!held, "untracked and invalid codes ignored");
    for (unsigned i = 0; i < 5; ++i) {
        state.apply({EventType::Button, i, true}, 2, start);
        for (unsigned j = 0; j < 5; ++j) check(state.buttons[j] == (i == j), "matching mouse region only");
        key(W, true);
        check(state.keys[W] && state.buttons[i], "keyboard and mouse coexist");
        state.apply({EventType::Button, i, false}, 2, start);
        check(!state.buttons[i] && state.keys[W], "mouse release preserves key");
        state.clear();
    }
    state.apply({EventType::Button, 99, true}, 2, start);
    for (bool held : state.buttons) check(!held, "invalid button ignored");
    key(W, true, 1); key(W, true, 2); key(W, false, 1);
    check(state.keys[W], "one keyboard release preserves another keyboard hold");
    key(D, true, 1);
    state.remove_device(2);
    check(!state.keys[W] && state.keys[D], "disconnect removes only originating device");
    state.remove_device(999);
    check(state.keys[D], "unknown device removal harmless");
    state.apply({EventType::Button, 0, true}, 3, start);
    state.remove_device(3);
    check(!state.buttons[0] && state.keys[D], "mouse disconnect preserves keyboard");
    key(LeftShift, true); key(RightShift, true); key(LeftShift, false);
    check(!state.keys[LeftShift] && state.keys[RightShift], "modifier sides independent");
    state.apply({EventType::Wheel, 0, false, 0, 0, 120}, 2, start);
    check(state.wheel == 120, "wheel up");
    state.expire(start + 179ms); check(state.wheel == 120, "wheel pulse retained");
    state.expire(start + 180ms); check(state.wheel == 0, "wheel pulse expires deterministically");
    state.apply({EventType::Wheel, 0, false, 0, 0, -120}, 2, start);
    check(state.wheel == -120, "wheel down");
    state.apply({EventType::Motion, 0, false, -50, 100}, 2, start);
    check(state.mouse_x == -50 && state.mouse_y == 100 && state.keys[D], "motion preserves holds");
    std::array<bool, 256> observed_keys{};
    std::array<bool, 5> observed_buttons{};
    state.apply({EventType::Button, 0, true}, 2, start);
    state.reconcile(observed_keys, observed_buttons, start + 99ms);
    check(state.keys[D] && state.buttons[0], "recovery grace avoids fresh event races");
    observed_keys[D] = true;
    state.reconcile(observed_keys, observed_buttons, start + 100ms);
    check(state.keys[D] && !state.keys[RightShift] && !state.buttons[0], "recovery clears missed releases independently");
    state.reconcile(observed_keys, observed_buttons, start + 1h);
    check(state.keys[D], "long observable holds do not time out");
    observed_keys[W] = true; observed_buttons[1] = true;
    state.reconcile(observed_keys, observed_buttons, start + 200ms);
    check(!state.keys[W] && !state.buttons[1], "polling does not invent presses");
    state.clear();
    for (bool held : state.keys) check(!held, "interrupt/shutdown clears all keys");
    for (bool held : state.buttons) check(!held, "interrupt/shutdown clears all buttons");
    check(state.wheel == 0, "interrupt clears scroll");
    key(W, true); check(state.keys[W], "capture can restart after reset");
    key(W, false); check(!state.keys[W], "restart release works");
    state.apply({EventType::Button, 0, true}, 1, start);
    state.apply({EventType::Button, 0, true}, 2, start);
    state.apply({EventType::Button, 0, false}, 1, start);
    check(state.buttons[0], "one mouse release preserves another mouse hold");
    state.remove_device(2);
    check(!state.buttons[0], "last mouse removal clears hold");
    std::cout << "Input-state transitions passed\n";
}
