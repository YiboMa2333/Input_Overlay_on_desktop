// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include "input/raw_input.hpp"
#include "render/renderer.hpp"
#include "settings/settings.hpp"
namespace overlay {
class Window {
    HWND handle_{};
    InputState state_;
    RawInput capture_;
    Renderer renderer_;
    Settings settings_;
    bool attached_{};
    bool session_notifications_{};
    bool suspended_{};
    bool locked_{}, disconnected_{}, sleeping_{};
    void suspend_capture();
    void resume_capture();
    static LRESULT CALLBACK procedure(HWND, UINT, WPARAM, LPARAM);
    LRESULT message(UINT, WPARAM, LPARAM);
public:
    int run(HINSTANCE instance, int show);
};
}
