// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <windows.h>
#include <sddl.h>
#include <filesystem>
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
namespace overlay {
struct InstanceIdentity { std::wstring key;GUID icon{};UINT activate{}; };
inline InstanceIdentity instance_identity() {
    HANDLE token{};
    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token)) return {};
    DWORD bytes{};GetTokenInformation(token,TokenUser,nullptr,0,&bytes);
    std::vector<BYTE> buffer(bytes);
    if(!GetTokenInformation(token,TokenUser,buffer.data(),bytes,&bytes)) {CloseHandle(token);return {};}
    CloseHandle(token);LPWSTR sid{};
    if(!ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER*>(buffer.data())->User.Sid,&sid)) return {};
    std::wstring identity=sid;LocalFree(sid);
    wchar_t path[32768]{};const auto size=GetEnvironmentVariableW(L"LOCALAPPDATA",path,32768);
    if(size&&size<32768) identity+=L"|"+std::filesystem::path(path).lexically_normal().wstring();
    CharLowerBuffW(identity.data(),static_cast<DWORD>(identity.size()));
    std::uint64_t hash=14695981039346656037ull;
    for(auto c:identity) {hash^=static_cast<unsigned>(c);hash*=1099511628211ull;}
    InstanceIdentity result;result.key=L"InputOverlayDesktop."+std::to_wstring(hash);
    result.activate=RegisterWindowMessageW((result.key+L".Activate").c_str());
    result.icon={0x77dcab11,0x8b75,0x4dc1,{}};std::memcpy(result.icon.Data4,&hash,sizeof(hash));
    return result;
}
class SingleInstance {
    HANDLE mutex_{};bool existing_{};
public:
    InstanceIdentity identity=instance_identity();
    SingleInstance() {
        if(identity.key.empty()||!identity.activate) return;
        mutex_=CreateMutexW(nullptr,FALSE,(L"Local\\"+identity.key).c_str());
        existing_=mutex_&&GetLastError()==ERROR_ALREADY_EXISTS;
    }
    ~SingleInstance() {if(mutex_) CloseHandle(mutex_);}
    bool valid() const {return mutex_!=nullptr;}
    bool existing() const {return existing_;}
    bool activate() const {
        struct Search {const wchar_t* key;HWND found;};Search search{identity.key.c_str(),nullptr};
        for(int i=0;i<100&&!search.found;++i) {
            EnumWindows([](HWND window,LPARAM data)->BOOL {
                auto search=reinterpret_cast<Search*>(data);
                if(GetPropW(window,search->key)) {search->found=window;return FALSE;}return TRUE;
            },reinterpret_cast<LPARAM>(&search));
            if(!search.found) Sleep(50);
        }
        if(!search.found) return false;
        DWORD pid{};GetWindowThreadProcessId(search.found,&pid);AllowSetForegroundWindow(pid);
        DWORD_PTR result{};return SendMessageTimeoutW(search.found,identity.activate,0,0,SMTO_ABORTIFHUNG,2000,&result)!=0;
    }
};
}
