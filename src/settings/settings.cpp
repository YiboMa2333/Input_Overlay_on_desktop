// SPDX-License-Identifier: GPL-2.0-only
#include "settings.hpp"
#include <windows.h>
#include <algorithm>
#include <filesystem>
#include <cwchar>
#include <cerrno>
namespace overlay {
bool Settings::valid_hotkey(unsigned modifiers, unsigned vk) {
    // Require at least two Ctrl/Alt/Shift modifiers and a function key. Avoid
    // tracked gaming keys, Windows-reserved combinations, F12 and bare keys.
    return (modifiers == 3 || modifiers == 5 || modifiers == 6 || modifiers == 7)
        && vk >= VK_F1 && vk <= VK_F11;
}
Settings Settings::load() {
    Settings result;
    wchar_t path[32768]{};
    DWORD count = GetEnvironmentVariableW(L"LOCALAPPDATA", path, 32768);
    if (!count || count >= 32768) return result;
    auto directory = std::filesystem::path(path) / L"InputOverlayDesktop";
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error) return result;
    result.file = (directory / L"settings.ini").wstring();
    auto read = [&](const wchar_t* name, int fallback, int low, int high) {
        wchar_t value[64]{};
        if (!GetPrivateProfileStringW(L"window", name, L"", value, 64, result.file.c_str())) return fallback;
        wchar_t* end{}; errno = 0;
        const auto number = std::wcstoll(value, &end, 10);
        if (errno || end == value || *end || number < low || number > high) return fallback;
        return static_cast<int>(number);
    };
    result.x = read(L"x", 60, -1000000, 1000000);
    result.y = read(L"y", 60, -1000000, 1000000);
    result.width = read(L"width", 338, 169, 2028);
    result.height = read(L"height", 148, 74, 888);
    result.edit = read(L"edit", 0, 0, 1) != 0;
    result.aspect_lock = read(L"aspect_lock", 1, 0, 1) != 0;
    result.hotkey_enabled = read(L"hotkey_enabled", 1, 0, 1) != 0;
    result.hotkey_modifiers = read(L"hotkey_modifiers", 3, 0, 7);
    result.hotkey_vk = read(L"hotkey_vk", 119, 0, 255);
    // Invalid requested shortcuts are disabled, never silently substituted.
    if (!valid_hotkey(result.hotkey_modifiers, result.hotkey_vk)) result.hotkey_enabled = false;
    return result;
}
bool Settings::save() const {
    if (file.empty()) return false;
    bool ok = true;
    auto write = [&](const wchar_t* key, int value) {
        if (!WritePrivateProfileStringW(L"window", key, std::to_wstring(value).c_str(), file.c_str())) ok = false;
    };
    write(L"x", x); write(L"y", y); write(L"width", width); write(L"height", height);
    write(L"edit", edit); write(L"aspect_lock", aspect_lock);
    write(L"hotkey_enabled", hotkey_enabled); write(L"hotkey_modifiers", hotkey_modifiers); write(L"hotkey_vk", hotkey_vk);
    return ok;
}
}
