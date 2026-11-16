// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <windows.h>
#include <objbase.h>
#include <oleauto.h>
// WIN32_LEAN_AND_MEAN excludes COM declarations from windows.h. Include them
// explicitly before the SDK's UI Automation interfaces (also needed by MSVC).
#include <uiautomation.h>
#include <string>
#include "render/com_ptr.hpp"
inline HWND own_popup_menu(HWND owner) {
    HWND result{};
    EnumThreadWindows(GetWindowThreadProcessId(owner,nullptr),[](HWND w,LPARAM data)->BOOL {
        wchar_t klass[64]{};GetClassNameW(w,klass,64);
        if(IsWindowVisible(w)&&std::wstring(klass)==L"#32768") {*reinterpret_cast<HWND*>(data)=w;return FALSE;}return TRUE;
    },reinterpret_cast<LPARAM>(&result));return result;
}
inline bool invoke_popup_item(HWND owner,const wchar_t* name) {
    const auto initialized=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);bool invoked=false;
    {
        overlay::ComPtr<IUIAutomation> automation;overlay::ComPtr<IUIAutomationElement> menu,item;
        overlay::ComPtr<IUIAutomationCondition> condition;overlay::ComPtr<IUIAutomationInvokePattern> invoke;
        VARIANT value{};value.vt=VT_BSTR;value.bstrVal=SysAllocString(name);
        if(SUCCEEDED(CoCreateInstance(__uuidof(CUIAutomation),nullptr,CLSCTX_INPROC_SERVER,__uuidof(IUIAutomation),reinterpret_cast<void**>(automation.GetAddressOf())))
            &&SUCCEEDED(automation->ElementFromHandle(own_popup_menu(owner),menu.GetAddressOf()))
            &&SUCCEEDED(automation->CreatePropertyCondition(UIA_NamePropertyId,value,condition.GetAddressOf()))
            &&SUCCEEDED(menu->FindFirst(TreeScope_Descendants,condition.Get(),item.GetAddressOf()))&&item
            &&SUCCEEDED(item->GetCurrentPatternAs(UIA_InvokePatternId,__uuidof(IUIAutomationInvokePattern),reinterpret_cast<void**>(invoke.GetAddressOf())))) invoked=SUCCEEDED(invoke->Invoke());
        VariantClear(&value);
    }
    if(SUCCEEDED(initialized)) CoUninitialize();return invoked;
}
// Test-only automation of the system hidden-icons button. No app depends on UIA.
inline bool open_notification_overflow() {
    const auto initialized=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    bool opened=false;
    {
        overlay::ComPtr<IUIAutomation> automation;
        if(SUCCEEDED(CoCreateInstance(__uuidof(CUIAutomation),nullptr,CLSCTX_INPROC_SERVER,
            __uuidof(IUIAutomation),reinterpret_cast<void**>(automation.GetAddressOf())))) {
            overlay::ComPtr<IUIAutomationElement> taskbar;
            if(SUCCEEDED(automation->ElementFromHandle(FindWindowW(L"Shell_TrayWnd",nullptr),taskbar.GetAddressOf()))) {
                VARIANT value{};value.vt=VT_I4;value.lVal=UIA_ButtonControlTypeId;
                overlay::ComPtr<IUIAutomationCondition> condition;
                overlay::ComPtr<IUIAutomationElementArray> buttons;
                if(SUCCEEDED(automation->CreatePropertyCondition(UIA_ControlTypePropertyId,value,condition.GetAddressOf()))
                    &&SUCCEEDED(taskbar->FindAll(TreeScope_Descendants,condition.Get(),buttons.GetAddressOf()))) {
                    int count{};buttons->get_Length(&count);
                    for(int i=0;i<count&&!opened;++i) {
                        overlay::ComPtr<IUIAutomationElement> button;buttons->GetElement(i,button.GetAddressOf());
                        BSTR name{};button->get_CurrentName(&name);std::wstring text=name?name:L"";SysFreeString(name);
                        if(text.find(L"Show hidden icons")==std::wstring::npos
                            &&text.find(L"\u663e\u793a\u9690\u85cf\u7684\u56fe\u6807")==std::wstring::npos) continue;
                        overlay::ComPtr<IUIAutomationInvokePattern> invoke;
                        if(SUCCEEDED(button->GetCurrentPatternAs(UIA_InvokePatternId,__uuidof(IUIAutomationInvokePattern),
                            reinterpret_cast<void**>(invoke.GetAddressOf())))) opened=SUCCEEDED(invoke->Invoke());
                    }
                }
            }
        }
    }
    if(SUCCEEDED(initialized)) CoUninitialize();return opened;
}
