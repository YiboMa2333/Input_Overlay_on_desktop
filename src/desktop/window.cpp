// SPDX-License-Identifier: GPL-2.0-only
#include "window.hpp"
namespace overlay {
LRESULT CALLBACK Window::procedure(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    auto self = reinterpret_cast<Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (msg == WM_NCCREATE) {
        self = static_cast<Window*>(reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams);
        self->handle_ = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    return self ? self->message(msg, w, l) : DefWindowProcW(hwnd, msg, w, l);
}
LRESULT Window::message(UINT msg, WPARAM w, LPARAM l) {
    switch (msg) {
    case WM_INPUT:
        capture_.handle(reinterpret_cast<HRAWINPUT>(l), state_);
        // DefWindowProc performs foreground RAWINPUT cleanup.
        return DefWindowProcW(handle_, msg, w, l);
    case WM_TIMER:
        state_.expire(); InvalidateRect(handle_, nullptr, FALSE); return 0;
    case WM_SIZE:
        renderer_.resize(LOWORD(l), HIWORD(l)); return 0;
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: {
        PAINTSTRUCT paint{}; BeginPaint(handle_, &paint);
        auto hr = renderer_.draw(handle_, state_);
        EndPaint(handle_, &paint);
        if (FAILED(hr)) { MessageBoxW(handle_, L"Direct2D rendering failed.", L"Input Overlay", MB_ICONERROR); DestroyWindow(handle_); }
        return 0;
    }
    case WM_DESTROY: {
        KillTimer(handle_, 1);
        if (attached_) { capture_.detach(); attached_ = false; }
        RECT rect{};
        if (!IsIconic(handle_) && GetWindowRect(handle_, &rect)) settings_.save_position(rect.left, rect.top);
        PostQuitMessage(0); return 0;
    }
    }
    return DefWindowProcW(handle_, msg, w, l);
}
int Window::run(HINSTANCE instance, int show) {
    settings_ = Settings::load();
    if (FAILED(renderer_.initialize())) {
        MessageBoxW(nullptr, L"Cannot initialize Direct2D/DirectWrite.", L"Input Overlay", MB_ICONERROR); return 1;
    }
    WNDCLASSW klass{};
    klass.lpfnWndProc = procedure; klass.hInstance = instance;
    klass.lpszClassName = L"InputOverlayDesktopWindow"; klass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    if (!RegisterClassW(&klass)) return 1;
    RECT position{settings_.x, settings_.y, settings_.x + settings_.width, settings_.y + settings_.height};
    if (!MonitorFromRect(&position, MONITOR_DEFAULTTONULL)) { settings_.x = 60; settings_.y = 60; }
    handle_ = CreateWindowExW(settings_.topmost ? WS_EX_TOPMOST : 0, klass.lpszClassName,
        L"Input Overlay Desktop", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        settings_.x, settings_.y, settings_.width, settings_.height, nullptr, nullptr, instance, this);
    if (!handle_) return 1;
    attached_ = capture_.attach(handle_);
    if (!attached_ || !SetTimer(handle_, 1, 16, nullptr)) {
        MessageBoxW(handle_, L"Cannot start input capture or refresh timer.", L"Input Overlay", MB_ICONERROR);
        DestroyWindow(handle_); return 1;
    }
    ShowWindow(handle_, show);
    MSG msg{};
    int result;
    while ((result = GetMessageW(&msg, nullptr, 0, 0)) > 0) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    return result < 0 ? 1 : static_cast<int>(msg.wParam);
}
}
