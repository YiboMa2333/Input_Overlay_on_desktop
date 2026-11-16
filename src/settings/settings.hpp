// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <string>
namespace overlay {
struct Settings {
    int x = 60, y = 60, width = 338, height = 148;
    bool topmost = true;
    std::wstring file;
    static Settings load();
    void save_position(int new_x, int new_y) const;
};
}
