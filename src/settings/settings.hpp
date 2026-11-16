// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <string>
#include <array>
namespace overlay {
struct KeyPreference { bool enabled=true; int x{}, y{}; };
struct Settings {
    int x = 60, y = 60, width = 338, height = 148;
    bool edit = false, aspect_lock = true, hotkey_enabled = true;
    unsigned hotkey_modifiers = 3, hotkey_vk = 0x77; // Ctrl+Alt+F8
    unsigned pressed_color=0xD8CF45, border_color=0xFFFFFF;
    int border_tenths=12, background_opacity=74, overlay_opacity=100;
    std::wstring font=L"Segoe UI";
    int font_size=13, scale=100, spacing=32;
    bool show_mouse=true, always_on_top=true, start_windows=false, launch_tray=false;
    int preset=0;
    std::array<KeyPreference,9> keys{{{true,70,8},{true,116,8},{true,162,8},
        {true,8,54},{true,70,54},{true,116,54},{true,162,54},{true,8,100},{true,70,100}}};
    std::wstring file;
    static Settings load();
    bool save() const;
    static bool valid_hotkey(unsigned modifiers, unsigned vk);
    void layout_preset(int index);
    void validate();
};
}
