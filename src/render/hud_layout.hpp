// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <array>
#include "input/input_state.hpp"
namespace overlay::hud {
inline constexpr float width = 338, height = 148;
struct KeyShape {
    const wchar_t* label;
    float x, y, width, height;
    unsigned primary, alternate;
    bool pressed(const InputState& state) const { return state.keys[primary] || state.keys[alternate]; }
};
inline constexpr std::array<KeyShape, 9> keys{{
    {L"Q", 70, 8, 40, 40, Q, Q}, {L"W", 116, 8, 40, 40, W, W}, {L"E", 162, 8, 40, 40, E, E},
    {L"Shift", 8, 54, 56, 40, LeftShift, RightShift},
    {L"A", 70, 54, 40, 40, A, A}, {L"S", 116, 54, 40, 40, S, S}, {L"D", 162, 54, 40, 40, D, D},
    {L"Ctrl", 8, 100, 56, 40, LeftCtrl, RightCtrl}, {L"Space", 70, 100, 132, 40, Space, Space}
}};
struct Fit { float scale, x, y; };
inline Fit fit(float pixel_width, float pixel_height) {
    const float a = pixel_width / width, b = pixel_height / height;
    const float scale = a < b ? a : b;
    return {scale, (pixel_width - width * scale) / 2, (pixel_height - height * scale) / 2};
}
inline bool hit(float x, float y) {
    for (const auto& key : keys)
        if (x >= key.x && x <= key.x + key.width && y >= key.y && y <= key.y + key.height) return true;
    return x >= 234 && x <= 322 && y >= 8 && y <= 140;
}
}
