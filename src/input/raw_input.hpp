// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <windows.h>
#include "input_state.hpp"

namespace overlay {
class RawInput {
public:
    bool attach(HWND window);
    void detach();
    void handle(HRAWINPUT handle, InputState& state) const;
};
}
