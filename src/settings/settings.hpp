// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <string>
namespace overlay {
struct Settings {
    int x = 60, y = 60, width = 338, height = 148;
    bool edit = false, aspect_lock = true, hotkey_enabled = true;
    unsigned hotkey_modifiers = 3, hotkey_vk = 0x77; // Ctrl+Alt+F8
    std::wstring file;
    static Settings load();
    bool save() const;
    static bool valid_hotkey(unsigned modifiers, unsigned vk);
};
}
