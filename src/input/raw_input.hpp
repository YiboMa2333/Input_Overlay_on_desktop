// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <windows.h>
#include "input_state.hpp"

namespace overlay {
class RawInput {
    HWND window_{};
public:
    RawInput() = default;
    RawInput(const RawInput&) = delete;
    RawInput& operator=(const RawInput&) = delete;
    ~RawInput() { detach(); }
    bool attach(HWND window);
    void detach();
    void handle(HRAWINPUT handle, InputState& state) const;
    void recover(InputState& state) const;
};
}
