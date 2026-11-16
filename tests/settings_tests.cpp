// SPDX-License-Identifier: GPL-2.0-only
#include "settings/settings.hpp"
#include "settings/startup.hpp"
#include "render/hud_layout.hpp"
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
    s.pressed_color=0x12ABEF;s.border_color=0x654321;s.border_tenths=25;s.background_opacity=33;s.overlay_opacity=65;
    s.font=L"Consolas";s.font_size=18;s.scale=150;s.spacing=70;s.show_mouse=false;s.always_on_top=false;s.launch_tray=true;
    s.keys[0]={false,200,100};
    check(s.save(),"save complete settings");
    auto restored=overlay::Settings::load();
    check(restored.x==-1820 && restored.y==-250 && restored.width==507 && restored.height==222,"negative coordinates and logical dimensions roundtrip");
    check(restored.edit && !restored.aspect_lock && restored.hotkey_modifiers==6 && restored.hotkey_vk==VK_F9,"mode, aspect and shortcut roundtrip");
    check(restored.pressed_color==0x12ABEF && restored.border_color==0x654321 && restored.border_tenths==25
        &&restored.background_opacity==33&&restored.overlay_opacity==65,"appearance values roundtrip");
    check(restored.font==L"Consolas"&&restored.font_size==18&&restored.scale==150&&restored.spacing==70,"font and scale roundtrip");
    check(!restored.show_mouse&&!restored.always_on_top&&restored.launch_tray&&!restored.keys[0].enabled&&restored.keys[0].x==200,"layout and behavior roundtrip");
    auto write=[&](const wchar_t* key,const wchar_t* value) { WritePrivateProfileStringW(L"window",key,value,s.file.c_str()); };
    write(L"width",L"999999999999999999999");write(L"height",L"-5");write(L"x",L"garbage");write(L"y",L"2000000000");
    write(L"edit",L"2");write(L"hotkey_modifiers",L"0");write(L"hotkey_vk",L"87");
    restored=overlay::Settings::load();
    check(restored.width==338 && restored.height==148 && restored.x==60 && restored.y==60 && !restored.edit,"corrupt and excessive values fall back safely");
    check(!restored.hotkey_enabled,"unmodified gaming key cannot become global shortcut");
    check(!overlay::Settings::valid_hotkey(3,VK_F12) && !overlay::Settings::valid_hotkey(8,VK_F8),"reserved shortcuts rejected");
    check(overlay::Settings::valid_hotkey(3,VK_F8),"default Ctrl Alt F8 permitted");
    auto malformed=restored;malformed.pressed_color=0xffffffff;malformed.border_tenths=999;malformed.overlay_opacity=-1;
    malformed.font=L"bad\nfont";malformed.scale=100000;malformed.show_mouse=false;
    for(auto& key:malformed.keys) key={false,-500,9999};
    malformed.validate();
    check(malformed.pressed_color==0xD8CF45&&malformed.border_tenths==50&&malformed.overlay_opacity==5
        &&malformed.font==L"Segoe UI"&&malformed.scale==400,"appearance validation bounds");
    check(malformed.keys[1].enabled&&malformed.keys[1].x==8&&malformed.keys[1].y==600,"nonempty bounded layout recovery");
    malformed.layout_preset(1);check(!malformed.keys[0].enabled&&!malformed.keys[2].enabled,"WASD preset");
    malformed.layout_preset(2);check(malformed.keys[8].x>malformed.keys[0].x&&malformed.keys[8].y==8,"single-row arrangement");
    malformed.layout_preset(0);check(overlay::hud::extent(malformed).width==338,"default preset reset extent");
    const auto test_key=L"Software\\InputOverlayDesktopTests\\Startup-"+std::to_wstring(GetCurrentProcessId());
    check(overlay::set_startup(true,L"C:\\Test Folder\\overlay.exe",test_key.c_str()),"startup registration in isolated test key");
    HKEY registry{};RegOpenKeyExW(HKEY_CURRENT_USER,test_key.c_str(),0,KEY_READ,&registry);
    wchar_t command[512]{};DWORD size=sizeof(command);RegQueryValueExW(registry,L"InputOverlayDesktop",nullptr,nullptr,reinterpret_cast<BYTE*>(command),&size);RegCloseKey(registry);
    check(std::wstring(command)==L"\"C:\\Test Folder\\overlay.exe\" --tray","startup executable path quoted");
    check(overlay::set_startup(false,L"",test_key.c_str()),"startup test value removal");RegDeleteKeyW(HKEY_CURRENT_USER,test_key.c_str());
    check(!overlay::set_startup(true,L"bad\"path",test_key.c_str()),"startup rejects embedded quotes");
    auto row=overlay::Settings::load();row.layout_preset(2);row.show_mouse=false;row.scale=50;
    const auto extent=overlay::hud::extent(row);row.width=static_cast<int>(extent.width/2);row.height=static_cast<int>(extent.height/2);
    check(row.save(),"small row layout saves");
    check(overlay::Settings::load().height==28&&overlay::Settings::load().width==row.width,"mouse-free 50 percent row size restores");
    s.file=profile.wstring();check(!s.save(),"configuration write errors observable");
    std::cout<<"Settings validation and persistence passed\n";
}
