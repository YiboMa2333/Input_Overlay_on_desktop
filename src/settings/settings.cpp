// SPDX-License-Identifier: GPL-2.0-only
#include "settings.hpp"
#include <windows.h>
#include <algorithm>
#include <filesystem>
#include <cwchar>
#include <cerrno>
namespace overlay {
void Settings::layout_preset(int index) {
    const Settings defaults;
    keys=defaults.keys;show_mouse=true;spacing=32;preset=index;
    if(index==1) { // WASD with modifiers; Q/E disabled.
        keys[0].enabled=keys[2].enabled=false;
    } else if(index==2) { // A single row of the same physical controls.
        int x=8;
        for(unsigned i=0;i<keys.size();++i) { keys[i].x=x;keys[i].y=8;x+=(i==3||i==7?56:i==8?132:40)+6; }
    } else preset=0;
}
void Settings::validate() {
    if(pressed_color>0xFFFFFF) pressed_color=0xD8CF45;
    if(border_color>0xFFFFFF) border_color=0xFFFFFF;
    border_tenths=std::clamp(border_tenths,5,50);
    background_opacity=std::clamp(background_opacity,0,100);
    overlay_opacity=std::clamp(overlay_opacity,5,100);
    font_size=std::clamp(font_size,6,36);scale=std::clamp(scale,50,400);spacing=std::clamp(spacing,8,160);
    if(font.empty()||font.size()>63||font.find_first_of(L"\r\n\t")!=std::wstring::npos) font=L"Segoe UI";
    for(auto& key:keys) { key.x=std::clamp(key.x,8,1000);key.y=std::clamp(key.y,8,600); }
    bool any=show_mouse;for(const auto& key:keys) any=any||key.enabled;
    if(!any) keys[1].enabled=true; // Always retain an object to edit.
    if(!valid_hotkey(hotkey_modifiers,hotkey_vk)) hotkey_enabled=false;
}
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
    result.width = read(L"width", 338, 32, 6000);
    result.height = read(L"height", 148, 24, 4000);
    result.edit = read(L"edit", 0, 0, 1) != 0;
    result.aspect_lock = read(L"aspect_lock", 1, 0, 1) != 0;
    result.hotkey_enabled = read(L"hotkey_enabled", 1, 0, 1) != 0;
    result.hotkey_modifiers = read(L"hotkey_modifiers", 3, 0, 7);
    result.hotkey_vk = read(L"hotkey_vk", 119, 0, 255);
    result.pressed_color=read(L"pressed_color",0xD8CF45,0,0xFFFFFF);
    result.border_color=read(L"border_color",0xFFFFFF,0,0xFFFFFF);
    result.border_tenths=read(L"border_tenths",12,5,50);
    result.background_opacity=read(L"background_opacity",74,0,100);
    result.overlay_opacity=read(L"overlay_opacity",100,5,100);
    result.font_size=read(L"font_size",13,6,36);result.scale=read(L"scale",100,50,400);
    result.spacing=read(L"spacing",32,8,160);
    result.show_mouse=read(L"show_mouse",1,0,1)!=0;
    result.always_on_top=read(L"always_on_top",1,0,1)!=0;
    result.start_windows=read(L"start_windows",0,0,1)!=0;
    result.launch_tray=read(L"launch_tray",0,0,1)!=0;
    result.preset=read(L"preset",0,0,2);
    wchar_t family[256]{};GetPrivateProfileStringW(L"window",L"font",L"Segoe UI",family,256,result.file.c_str());result.font=family;
    for(unsigned i=0;i<result.keys.size();++i) {
        const auto prefix=L"key"+std::to_wstring(i)+L"_";
        result.keys[i].enabled=read((prefix+L"enabled").c_str(),1,0,1)!=0;
        result.keys[i].x=read((prefix+L"x").c_str(),result.keys[i].x,8,1000);
        result.keys[i].y=read((prefix+L"y").c_str(),result.keys[i].y,8,600);
    }
    result.validate();
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
    write(L"pressed_color",pressed_color);write(L"border_color",border_color);write(L"border_tenths",border_tenths);
    write(L"background_opacity",background_opacity);write(L"overlay_opacity",overlay_opacity);
    write(L"font_size",font_size);write(L"scale",scale);write(L"spacing",spacing);
    write(L"show_mouse",show_mouse);write(L"always_on_top",always_on_top);write(L"start_windows",start_windows);
    write(L"launch_tray",launch_tray);write(L"preset",preset);
    if(!WritePrivateProfileStringW(L"window",L"font",font.c_str(),file.c_str())) ok=false;
    for(unsigned i=0;i<keys.size();++i) {
        const auto prefix=L"key"+std::to_wstring(i)+L"_";
        write((prefix+L"enabled").c_str(),keys[i].enabled);
        write((prefix+L"x").c_str(),keys[i].x);write((prefix+L"y").c_str(),keys[i].y);
    }
    return ok;
}
}
