// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include "input/raw_input.hpp"
#include "render/renderer.hpp"
#include "settings/settings.hpp"
#include "settings/dialog.hpp"
#include "geometry.hpp"
#include "instance.hpp"
#include <shellapi.h>
namespace overlay {
class Window {
    HWND handle_{};
    HWND previous_foreground_{};
    InputState state_;
    RawInput capture_;
    Renderer renderer_;
    Settings settings_;
    SettingsDialog settings_dialog_;
    bool dirty_=true;
    InstanceIdentity identity_;
    HICON icon_{};
    bool tray_version4_{};
    bool menu_open_{};
    ULONGLONG last_right_menu_{};
    bool attached_{}, session_notifications_{}, suspended_{};
    bool session_locked_{}, disconnected_{}, sleeping_{};
    bool tray_added_{}, hotkey_registered_{}, save_failed_{};
    UINT taskbar_created_{};
    ULONGLONG next_tray_retry_{};
    placement::Handle dragging_ = placement::Handle::None;
    placement::Rect drag_start_{};
    POINT drag_origin_{};
    void suspend_capture();
    void resume_capture();
    void mode(bool edit,bool reveal=true);
    bool customize(Settings preferences,bool reset);
    void persist();
    void recover(bool reset = false);
    void apply(placement::Rect rect);
    placement::Rect bounds() const;
    void end_drag(bool cancel = false);
    void menu();
    void command(unsigned id);
    bool tray(bool add);
    void hotkey();
    static LRESULT CALLBACK procedure(HWND, UINT, WPARAM, LPARAM);
    LRESULT message(UINT, WPARAM, LPARAM);
public:
    ~Window() {if(icon_) DestroyIcon(icon_);}
    int run(HINSTANCE instance,int show,bool force_tray=false,InstanceIdentity identity={});
};
}
