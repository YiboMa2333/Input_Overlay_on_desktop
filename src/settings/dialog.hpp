// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <windows.h>
#include <functional>
#include <vector>
#include "settings.hpp"
namespace overlay {
namespace settings_control {
enum { Tab=500,Pressed=501,Border,Thickness,Background,Opacity,Family,FontSize,Scale,Spacing,FontPicker,
    Preset,KeySelect,KeyEnabled,KeyX,KeyY,Mouse,ResetLayout,Topmost,Edit,Aspect,Startup,Tray,
    Hotkey,Modifiers,FunctionKey,ResetGeometry,Apply,Close,Status };
}
class SettingsDialog {
    struct Control { HWND window; int page,x,y,w,h; };
    HWND window_{};HFONT font_{};Settings draft_;
    std::function<bool(Settings,bool)> apply_;
    std::vector<Control> controls_;
    int page_{},selected_key_{};bool reset_{};
    HWND add(const wchar_t*,const wchar_t*,int,int,int,int,int,int,DWORD=0);
    void build();void reflow();void populate();void editor();bool store_editor();bool collect();
    void color(bool pressed);void choose_font();
    static LRESULT CALLBACK procedure(HWND,UINT,WPARAM,LPARAM);
    LRESULT message(UINT,WPARAM,LPARAM);
public:
    ~SettingsDialog();
    void show(HWND owner,const Settings&,std::function<bool(Settings,bool)> callback);
    bool dispatch(MSG& msg) const;
    HWND handle() const { return window_; }
};
}
