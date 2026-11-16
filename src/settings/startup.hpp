// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <windows.h>
#include <string>
namespace overlay {
inline constexpr const wchar_t* startup_key=L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
// Alternate key path permits isolated tests without changing actual logon behavior.
inline bool set_startup(bool enabled,const std::wstring& executable,const wchar_t* key_path=startup_key) {
    if(enabled && (executable.empty() || executable.find(L'"')!=std::wstring::npos || executable.size()+11>260)) return false;
    HKEY key{};
    if(RegCreateKeyExW(HKEY_CURRENT_USER,key_path,0,nullptr,0,KEY_SET_VALUE,nullptr,&key,nullptr)!=ERROR_SUCCESS) return false;
    const auto command=L"\""+executable+L"\" --tray";
    const auto result=enabled ? RegSetValueExW(key,L"InputOverlayDesktop",0,REG_SZ,
        reinterpret_cast<const BYTE*>(command.c_str()),static_cast<DWORD>((command.size()+1)*sizeof(wchar_t)))
        : RegDeleteValueW(key,L"InputOverlayDesktop");
    RegCloseKey(key);return result==ERROR_SUCCESS||(!enabled&&result==ERROR_FILE_NOT_FOUND);
}
}
