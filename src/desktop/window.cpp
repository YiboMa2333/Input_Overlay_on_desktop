// SPDX-License-Identifier: GPL-2.0-only
#include "window.hpp"
#include "commands.hpp"
#include "render/hud_layout.hpp"
#include "settings/startup.hpp"
#include <wtsapi32.h>
#include <windowsx.h>
#include <string>
#include <cmath>
namespace overlay {
namespace {
constexpr UINT tray_message = WM_APP + 1;
constexpr int hotkey_id = 1;
constexpr const wchar_t* class_name = L"InputOverlayDesktopWindow";
std::vector<placement::Rect> screens() {
    std::vector<placement::Rect> result;
    EnumDisplayMonitors(nullptr, nullptr, [](HMONITOR monitor, HDC, LPRECT, LPARAM data) -> BOOL {
        MONITORINFO info{}; info.cbSize = sizeof(info);
        if (GetMonitorInfoW(monitor, &info)) {
            auto r = info.rcWork;
            reinterpret_cast<std::vector<placement::Rect>*>(data)->push_back(
                {double(r.left), double(r.top), double(r.right-r.left), double(r.bottom-r.top)});
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&result));
    return result;
}
LPCWSTR cursor(placement::Handle handle) {
    using H = placement::Handle;
    switch (handle) {
    case H::NW: case H::SE: return IDC_SIZENWSE;
    case H::NE: case H::SW: return IDC_SIZENESW;
    case H::N: case H::S: return IDC_SIZENS;
    case H::E: case H::W: return IDC_SIZEWE;
    case H::Move: return IDC_SIZEALL;
    default: return IDC_ARROW;
    }
}
}
placement::Rect Window::bounds() const {
    RECT r{}; GetWindowRect(handle_, &r);
    return {double(r.left), double(r.top), double(r.right-r.left), double(r.bottom-r.top)};
}
void Window::apply(placement::Rect r) {
    SetWindowPos(handle_, settings_.always_on_top?HWND_TOPMOST:HWND_NOTOPMOST, static_cast<int>(std::lround(r.x)), static_cast<int>(std::lround(r.y)),
        static_cast<int>(std::lround(r.w)), static_cast<int>(std::lround(r.h)), SWP_NOACTIVATE);
    InvalidateRect(handle_, nullptr, FALSE);
}
void Window::persist() {
    const auto r = bounds(); const auto dpi = GetDpiForWindow(handle_);
    if (!dpi || r.w <= 0 || r.h <= 0) return;
    settings_.x = static_cast<int>(r.x); settings_.y = static_cast<int>(r.y);
    settings_.width = MulDiv(static_cast<int>(r.w), 96, dpi);
    settings_.height = MulDiv(static_cast<int>(r.h), 96, dpi);
    settings_.scale=std::clamp(static_cast<int>(std::lround(settings_.width*100/hud::extent(settings_).width)),50,400);
    save_failed_ = !settings_.save();
}
void Window::recover(bool reset) {
    auto r = bounds();
    if (reset) {
        MONITORINFO info{}; info.cbSize = sizeof(info);
        GetMonitorInfoW(MonitorFromPoint({0,0}, MONITOR_DEFAULTTOPRIMARY), &info);
        const auto dpi = GetDpiForWindow(handle_);
        const auto extent=hud::extent(settings_);
        r = {double(info.rcWork.left+24), double(info.rcWork.top+24), extent.width*dpi/96, extent.height*dpi/96};
    }
    apply(placement::recover(r, screens(), reset));
    persist();
}
void Window::end_drag(bool cancel) {
    if (dragging_ == placement::Handle::None) return;
    dragging_ = placement::Handle::None;
    if (GetCapture() == handle_) ReleaseCapture();
    if (cancel) apply(drag_start_);
    recover(); // Keep partially visible geometry; fully inaccessible geometry is recovered.
}
bool Window::tray(bool add) {
    NOTIFYICONDATAW data{}; data.cbSize = sizeof(data); data.hWnd = handle_; data.uID = 1;
    data.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP;
    data.uCallbackMessage = tray_message;
    data.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    std::wstring tip = settings_.edit ? L"Input Overlay - Edit" : L"Input Overlay - Locked";
    if (settings_.hotkey_enabled && !hotkey_registered_) tip += L" (shortcut unavailable)";
    if (save_failed_) tip += L" (settings not saved)";
    tip.copy(data.szTip, 127); data.szTip[tip.size() < 127 ? tip.size() : 127] = 0;
    if (!Shell_NotifyIconW(add ? NIM_ADD : NIM_MODIFY, &data)) return false;
    if (add) { data.uVersion = NOTIFYICON_VERSION_4; Shell_NotifyIconW(NIM_SETVERSION, &data); }
    tray_added_ = true; return true;
}
void Window::hotkey() {
    if (hotkey_registered_) UnregisterHotKey(handle_, hotkey_id);
    hotkey_registered_ = settings_.hotkey_enabled
        && Settings::valid_hotkey(settings_.hotkey_modifiers, settings_.hotkey_vk)
        && RegisterHotKey(handle_, hotkey_id, settings_.hotkey_modifiers | MOD_NOREPEAT, settings_.hotkey_vk);
}
void Window::mode(bool edit,bool reveal) {
    end_drag(); settings_.edit = edit;
    auto style = GetWindowLongPtrW(handle_, GWL_EXSTYLE);
    // Layered WS_EX_TRANSPARENT skips this HWND during hit testing across
    // processes. HTTRANSPARENT alone only forwards within the same thread.
    if (edit) style &= ~(WS_EX_TRANSPARENT | WS_EX_NOACTIVATE);
    else style |= WS_EX_TRANSPARENT | WS_EX_NOACTIVATE;
    SetWindowLongPtrW(handle_, GWL_EXSTYLE, style);
    SetWindowPos(handle_, settings_.always_on_top?HWND_TOPMOST:HWND_NOTOPMOST, 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    if(reveal) ShowWindow(handle_,SW_SHOWNOACTIVATE);
    // Remove edit alpha catcher and controls immediately, before returning to the user.
    if(renderer_.draw(handle_,state_,settings_.edit)==S_FALSE) dirty_=true;
    persist(); tray(false);
    if (!edit && GetForegroundWindow() == handle_ && IsWindow(previous_foreground_))
        SetForegroundWindow(previous_foreground_);
}
bool Window::customize(Settings preferences,bool reset) {
    end_drag();preferences.validate();
    if(FAILED(renderer_.configure(preferences))) return false;
    if(preferences.start_windows!=settings_.start_windows) {
        wchar_t executable[32768]{};const auto count=GetModuleFileNameW(nullptr,executable,32768);
        if(!count||count>=32768||!set_startup(preferences.start_windows,executable)) {renderer_.configure(settings_);return false;}
    }
    const auto old=hud::extent(settings_),next=hud::extent(preferences);
    const bool resize=old.width!=next.width||old.height!=next.height||settings_.scale!=preferences.scale;
    preferences.file=settings_.file;settings_=preferences;
    if(reset) recover(true);
    else if(resize) {
        auto r=bounds();const auto dpi=GetDpiForWindow(handle_);
        r.w=next.width*settings_.scale*dpi/9600;r.h=next.height*settings_.scale*dpi/9600;apply(r);
    }
    hotkey();mode(settings_.edit);dirty_=true;InvalidateRect(handle_,nullptr,FALSE);
    if(settings_.hotkey_enabled&&!hotkey_registered_) MessageBoxW(settings_dialog_.handle(),L"Settings were applied, but this shortcut is unavailable. Choose another combination; tray controls still work.",L"Shortcut conflict",MB_ICONINFORMATION);
    return !save_failed_;
}
void Window::menu() {
    HMENU popup = CreatePopupMenu(), options = CreatePopupMenu();
    if (!popup || !options) { if(popup) DestroyMenu(popup); if(options) DestroyMenu(options); return; }
    AppendMenuW(popup, MF_STRING | (settings_.edit ? MF_CHECKED : 0), command::edit, L"Edit Overlay");
    AppendMenuW(popup, MF_STRING | (!settings_.edit ? MF_CHECKED : 0), command::lock, L"Lock Overlay");
    AppendMenuW(popup, MF_STRING, command::reset, L"Reset Position");
    AppendMenuW(options, MF_STRING | (settings_.aspect_lock ? MF_CHECKED : 0), command::aspect, L"Lock aspect ratio");
    AppendMenuW(options, MF_STRING, command::settings, L"Customize overlay...");
    AppendMenuW(options, MF_STRING, command::reload, L"Reload configuration");
    const auto status = !settings_.hotkey_enabled ? L"Shortcut: disabled" : hotkey_registered_ ? L"Shortcut: registered" : L"Shortcut: unavailable (conflict)";
    AppendMenuW(options, MF_STRING | MF_GRAYED, 0, status);
    if (save_failed_) AppendMenuW(options, MF_STRING | MF_GRAYED, 0, L"Configuration write failed");
    AppendMenuW(popup, MF_POPUP, reinterpret_cast<UINT_PTR>(options), L"Settings");
    AppendMenuW(popup, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(popup, MF_STRING, command::exit, L"Exit");
    POINT point{}; GetCursorPos(&point);
    if (GetForegroundWindow() != handle_) previous_foreground_ = GetForegroundWindow();
    SetForegroundWindow(handle_);
    auto selected = TrackPopupMenu(popup, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON,
        point.x, point.y, 0, handle_, nullptr);
    DestroyMenu(popup);
    if (selected) command(selected);
    PostMessageW(handle_, WM_NULL, 0, 0);
}
void Window::command(unsigned id) {
    switch(id) {
    case command::edit: mode(true); break;
    case command::lock: mode(false); break;
    case command::reset: end_drag(); recover(true); break;
    case command::aspect: settings_.aspect_lock = !settings_.aspect_lock; persist(); break;
    case command::settings:
        persist();settings_dialog_.show(handle_,settings_,[this](Settings preferences,bool reset){return customize(std::move(preferences),reset);});
        break;
    case command::reload: {
        end_drag(); settings_ = Settings::load();
        const auto dpi = GetDpiForWindow(handle_);
        apply({double(settings_.x),double(settings_.y),double(MulDiv(settings_.width,dpi,96)),double(MulDiv(settings_.height,dpi,96))});
        renderer_.configure(settings_);hotkey(); recover(); mode(settings_.edit); break;
    }
    case command::exit: DestroyWindow(handle_); break;
    }
}
void Window::suspend_capture() {
    end_drag(); suspended_ = true; capture_.detach(); attached_ = false; state_.clear();
    InvalidateRect(handle_, nullptr, FALSE);
}
void Window::resume_capture() {
    if (session_locked_ || disconnected_ || sleeping_) return;
    state_.clear(); suspended_ = false; attached_ = capture_.attach(handle_);
    if (!attached_) { MessageBoxW(handle_, L"Cannot resume input capture.", L"Input Overlay", MB_ICONERROR); DestroyWindow(handle_); }
}
LRESULT CALLBACK Window::procedure(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    auto self = reinterpret_cast<Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (msg == WM_NCCREATE) {
        self = static_cast<Window*>(reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams);
        self->handle_ = hwnd; SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    return self ? self->message(msg,w,l) : DefWindowProcW(hwnd,msg,w,l);
}
LRESULT Window::message(UINT msg, WPARAM w, LPARAM l) {
    if (taskbar_created_ && msg == taskbar_created_) { tray_added_ = false; tray(true); return 0; }
    switch(msg) {
    case tray_message:
        if (LOWORD(l) == WM_CONTEXTMENU || LOWORD(l) == NIN_SELECT || LOWORD(l) == NIN_KEYSELECT) menu();
        return 0;
    case WM_COMMAND: command(LOWORD(w)); return 0;
    case WM_HOTKEY: if (w == hotkey_id) mode(!settings_.edit); return 0;
    case WM_MOUSEACTIVATE:
        if(settings_.edit && GetForegroundWindow() != handle_) previous_foreground_ = GetForegroundWindow();
        return settings_.edit ? MA_ACTIVATE : MA_NOACTIVATE;
    case WM_NCHITTEST: return settings_.edit ? HTCLIENT : HTTRANSPARENT;
    case WM_SETCURSOR: {
        POINT p{}; GetCursorPos(&p); ScreenToClient(handle_,&p); auto r = bounds();
        const auto handle = dragging_ != placement::Handle::None ? dragging_
            : placement::hit({double(p.x),double(p.y)},r.w,r.h,GetDpiForWindow(handle_)/96.0);
        SetCursor(LoadCursorW(nullptr, settings_.edit ? cursor(handle) : IDC_ARROW)); return TRUE;
    }
    case WM_LBUTTONDOWN:
        if (settings_.edit) {
            drag_start_ = bounds();
            // Anchor to the press event, not a later asynchronous cursor query.
            // Fast input/queueing can move the cursor before this message runs.
            const auto position = GetMessagePos();
            drag_origin_ = {GET_X_LPARAM(position),GET_Y_LPARAM(position)};
            dragging_ = placement::hit({double(GET_X_LPARAM(l)),double(GET_Y_LPARAM(l))},
                drag_start_.w,drag_start_.h,GetDpiForWindow(handle_)/96.0);
            SetCapture(handle_);
        }
        return 0;
    case WM_MOUSEMOVE:
        if (dragging_ != placement::Handle::None && GetCapture() == handle_) {
            // GetMessagePos is the queued event's screen point, independent of
            // current window origin and later asynchronous pointer movement.
            const auto position = GetMessagePos();
            POINT p{GET_X_LPARAM(position),GET_Y_LPARAM(position)};
            const double dpi = GetDpiForWindow(handle_)/96.0;
            const auto extent=hud::extent(settings_);
            apply(placement::drag(drag_start_,dragging_,p.x-drag_origin_.x,p.y-drag_origin_.y,
                settings_.aspect_lock,(w&MK_SHIFT)!=0,{std::max(32.0,extent.width*0.5)*dpi,
                    std::max(24.0,extent.height*0.5)*dpi,std::min(6000.0,extent.width*4.0)*dpi,
                    std::min(4000.0,extent.height*4.0)*dpi},extent.width/extent.height));
        }
        return 0;
    case WM_LBUTTONUP: end_drag(); return 0;
    case WM_CAPTURECHANGED: if (dragging_ != placement::Handle::None) end_drag(); return 0;
    case WM_CANCELMODE: end_drag(true); return 0;
    case WM_KEYDOWN: if (w == VK_ESCAPE) end_drag(true); return 0;
    case WM_RBUTTONUP: if(settings_.edit) menu(); return 0;
    case WM_DPICHANGED: {
        // A captured drag crossing monitors must rebase, not apply old-DPI deltas.
        end_drag(); const auto r = reinterpret_cast<const RECT*>(l);
        apply({double(r->left),double(r->top),double(r->right-r->left),double(r->bottom-r->top)});
        persist(); return 0;
    }
    case WM_DISPLAYCHANGE: end_drag(); recover(); return 0;
    case WM_SETTINGCHANGE: if(w == SPI_SETWORKAREA) recover(); return 0;
    case WM_INPUT_DEVICE_CHANGE:
        if(w == GIDC_REMOVAL) state_.remove_device(static_cast<InputState::Device>(l));
        InvalidateRect(handle_,nullptr,FALSE); return 0;
    case WM_WTSSESSION_CHANGE:
        switch(w) {
        case WTS_SESSION_LOCK: session_locked_ = true; break;
        case WTS_SESSION_UNLOCK: session_locked_ = false; break;
        case WTS_CONSOLE_DISCONNECT: case WTS_REMOTE_DISCONNECT: disconnected_ = true; break;
        case WTS_CONSOLE_CONNECT: case WTS_REMOTE_CONNECT: disconnected_ = false; break;
        default: return 0;
        }
        if(session_locked_ || disconnected_ || sleeping_) suspend_capture(); else resume_capture();
        return 0;
    case WM_POWERBROADCAST:
        if(w == PBT_APMSUSPEND) { sleeping_ = true; suspend_capture(); }
        if(w == PBT_APMRESUMEAUTOMATIC) { sleeping_ = false; resume_capture(); recover(); }
        return TRUE;
    case WM_ACTIVATEAPP: if(attached_) capture_.recover(state_); return 0;
    case WM_INPUT:
    {   const auto keys=state_.keys;const auto buttons=state_.buttons;const auto wheel=state_.wheel;
        capture_.handle(reinterpret_cast<HRAWINPUT>(l),state_);
        dirty_=dirty_||keys!=state_.keys||buttons!=state_.buttons||wheel!=state_.wheel;
        return DefWindowProcW(handle_,msg,w,l);
    }
    case WM_TIMER:
    {   const auto keys=state_.keys;const auto buttons=state_.buttons;const auto wheel=state_.wheel;
        if(!suspended_ && attached_) capture_.recover(state_);
        state_.expire();dirty_=dirty_||keys!=state_.keys||buttons!=state_.buttons||wheel!=state_.wheel;
        if(dirty_&&IsWindowVisible(handle_)) InvalidateRect(handle_,nullptr,FALSE);
        if(!tray_added_ && GetTickCount64() >= next_tray_retry_) { tray(true); next_tray_retry_ = GetTickCount64()+2000; }
        return 0;
    }
    case WM_SIZE: renderer_.resize(LOWORD(l),HIWORD(l)); InvalidateRect(handle_,nullptr,FALSE); return 0;
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: {
        PAINTSTRUCT p{}; BeginPaint(handle_,&p);
        const auto hr = renderer_.draw(handle_,state_,settings_.edit,
            dragging_ != placement::Handle::None && dragging_ != placement::Handle::Move);
        EndPaint(handle_,&p);
        dirty_=hr==S_FALSE;
        if(FAILED(hr)) { MessageBoxW(handle_,L"Direct2D rendering failed.",L"Input Overlay",MB_ICONERROR); DestroyWindow(handle_); }
        return 0;
    }
    case WM_DESTROY: {
        persist(); KillTimer(handle_,1); capture_.detach(); attached_ = false; state_.clear();
        if(hotkey_registered_) UnregisterHotKey(handle_,hotkey_id);
        if(session_notifications_) WTSUnRegisterSessionNotification(handle_);
        NOTIFYICONDATAW icon{}; icon.cbSize = sizeof(icon); icon.hWnd = handle_; icon.uID = 1;
        Shell_NotifyIconW(NIM_DELETE,&icon); tray_added_ = false;
        PostQuitMessage(0); return 0;
    }
    }
    return DefWindowProcW(handle_,msg,w,l);
}
int Window::run(HINSTANCE instance, int show,bool force_tray) {
    settings_ = Settings::load();
    if(FAILED(renderer_.initialize())) { MessageBoxW(nullptr,L"Cannot initialize Direct2D.",L"Input Overlay",MB_ICONERROR); return 1; }
    if(FAILED(renderer_.configure(settings_))) return 1;
    WNDCLASSW klass{}; klass.lpfnWndProc = procedure; klass.hInstance = instance;
    klass.lpszClassName = class_name; klass.hCursor = LoadCursorW(nullptr,IDC_ARROW);
    if(!RegisterClassW(&klass)) return 1;
    handle_ = CreateWindowExW(WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | (settings_.always_on_top?WS_EX_TOPMOST:0)
        | (settings_.edit ? 0 : WS_EX_TRANSPARENT),class_name,L"Input Overlay Desktop",WS_POPUP,
        settings_.x,settings_.y,settings_.width,settings_.height,nullptr,nullptr,instance,this);
    if(!handle_) return 1;
    const auto dpi = GetDpiForWindow(handle_);
    apply({double(settings_.x),double(settings_.y),double(MulDiv(settings_.width,dpi,96)),double(MulDiv(settings_.height,dpi,96))});
    recover(); taskbar_created_ = RegisterWindowMessageW(L"TaskbarCreated");
    session_notifications_ = WTSRegisterSessionNotification(handle_,NOTIFY_FOR_THIS_SESSION)!=FALSE;
    attached_ = capture_.attach(handle_); hotkey();
    if(!session_notifications_ || !attached_ || !SetTimer(handle_,1,33,nullptr) || !tray(true)) {
        MessageBoxW(handle_,L"Cannot start input capture, timer, session notifications or recovery tray icon.",L"Input Overlay",MB_ICONERROR);
        DestroyWindow(handle_); return 1;
    }
    mode(settings_.edit,false);
    if(FAILED(renderer_.draw(handle_,state_,settings_.edit))) { DestroyWindow(handle_); return 1; }
    ShowWindow(handle_,show == SW_HIDE||force_tray||settings_.launch_tray ? SW_HIDE : SW_SHOWNOACTIVATE);
    MSG msg{}; int status;
    while((status = GetMessageW(&msg,nullptr,0,0)) > 0) {if(!settings_dialog_.dispatch(msg)) {TranslateMessage(&msg);DispatchMessageW(&msg);} }
    if(IsWindow(handle_)) DestroyWindow(handle_);
    return status < 0 ? 1 : static_cast<int>(msg.wParam);
}
}
