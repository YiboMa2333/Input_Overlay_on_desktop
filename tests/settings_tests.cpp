// SPDX-License-Identifier: GPL-2.0-only
#include "settings/settings.hpp"
#include <windows.h>
#include <filesystem>
#include <iostream>
#include <cstdlib>
namespace {
void check(bool value,const char* text) { if(!value) { std::cerr<<"FAIL: "<<text<<'\n';std::exit(1); } }
}
int main() {
    const auto profile=std::filesystem::absolute(std::filesystem::path("build")/("settings-profile-"+std::to_string(GetCurrentProcessId())));
    std::filesystem::create_directories(profile);
    SetEnvironmentVariableW(L"LOCALAPPDATA",profile.c_str());
    auto s=overlay::Settings::load();
    check(s.width==338 && s.height==148 && !s.edit && s.aspect_lock,"sane locked defaults");
    s.x=-1820;s.y=-250;s.width=507;s.height=222;s.edit=true;s.aspect_lock=false;
    s.hotkey_modifiers=6;s.hotkey_vk=VK_F9;
    check(s.save(),"save complete settings");
    auto restored=overlay::Settings::load();
    check(restored.x==-1820 && restored.y==-250 && restored.width==507 && restored.height==222,"negative coordinates and logical dimensions roundtrip");
    check(restored.edit && !restored.aspect_lock && restored.hotkey_modifiers==6 && restored.hotkey_vk==VK_F9,"mode, aspect and shortcut roundtrip");
    auto write=[&](const wchar_t* key,const wchar_t* value) { WritePrivateProfileStringW(L"window",key,value,s.file.c_str()); };
    write(L"width",L"999999999999999999999");write(L"height",L"-5");write(L"x",L"garbage");write(L"y",L"2000000000");
    write(L"edit",L"2");write(L"hotkey_modifiers",L"0");write(L"hotkey_vk",L"87");
    restored=overlay::Settings::load();
    check(restored.width==338 && restored.height==148 && restored.x==60 && restored.y==60 && !restored.edit,"corrupt and excessive values fall back safely");
    check(!restored.hotkey_enabled,"unmodified gaming key cannot become global shortcut");
    check(!overlay::Settings::valid_hotkey(3,VK_F12) && !overlay::Settings::valid_hotkey(8,VK_F8),"reserved shortcuts rejected");
    check(overlay::Settings::valid_hotkey(3,VK_F8),"default Ctrl Alt F8 permitted");
    s.file=profile.wstring();check(!s.save(),"configuration write errors observable");
    std::cout<<"Settings validation and persistence passed\n";
}
