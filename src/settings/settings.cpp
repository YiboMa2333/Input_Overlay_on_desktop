// SPDX-License-Identifier: GPL-2.0-only
#include "settings.hpp"
#include <windows.h>
#include <algorithm>
#include <filesystem>
namespace overlay {
Settings Settings::load() {
    Settings result;
    wchar_t path[32768]{};
    DWORD count = GetEnvironmentVariableW(L"LOCALAPPDATA", path, 32768);
    if (!count || count >= 32768) return result;
    auto directory = std::filesystem::path(path) / L"InputOverlayDesktop";
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error) return result; // Defaults remain usable if settings cannot be written.
    result.file = (directory / L"settings.ini").wstring();
    auto read = [&](const wchar_t* name, int fallback) {
        return static_cast<int>(GetPrivateProfileIntW(L"window", name, fallback, result.file.c_str()));
    };
    result.x = read(L"x", result.x); result.y = read(L"y", result.y);
    result.width = std::clamp(read(L"width", result.width), 254, 1352);
    result.height = std::clamp(read(L"height", result.height), 111, 592);
    result.topmost = read(L"topmost", 1) != 0;
    return result;
}
void Settings::save_position(int new_x, int new_y) const {
    if (file.empty()) return;
    WritePrivateProfileStringW(L"window", L"x", std::to_wstring(new_x).c_str(), file.c_str());
    WritePrivateProfileStringW(L"window", L"y", std::to_wstring(new_y).c_str(), file.c_str());
}
}
