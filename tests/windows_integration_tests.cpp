// SPDX-License-Identifier: GPL-2.0-only
// Interactive test: opens a dedicated backdrop, moves the pointer, and launches
// only this test's child overlay with an isolated LOCALAPPDATA configuration.
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <dlgs.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <cstdlib>
#include <array>
#include <utility>
#include <vector>
#include "desktop/commands.hpp"
#include "settings/dialog.hpp"
namespace {
// RGB edit controls in the legacy common color dialog; presence is checked.
constexpr int COLOR_RED=706,COLOR_GREEN=707,COLOR_BLUE=708;
int clicks{}, hotkeys{};
COLORREF backdrop_color=RGB(55,65,80);
HWND fixture{};
HWND overlay_window{};
PROCESS_INFORMATION child{};
POINT original_pointer{};
std::array<bool,256> injected_keys{};
bool injected_button{};
void pump(unsigned ms) {
    const auto until = GetTickCount64()+ms;
    do { MSG message{}; while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)) { TranslateMessage(&message); DispatchMessageW(&message); } Sleep(5); }
    while(GetTickCount64()<until);
}
void cleanup() {
    for(unsigned vk=0;vk<injected_keys.size();++vk) if(injected_keys[vk]) {
        INPUT input{};input.type=INPUT_KEYBOARD;input.ki.wVk=static_cast<WORD>(vk);input.ki.dwFlags=KEYEVENTF_KEYUP;
        SendInput(1,&input,sizeof(input));injected_keys[vk]=false;
    }
    if(injected_button) { INPUT input{};input.type=INPUT_MOUSE;input.mi.dwFlags=MOUSEEVENTF_LEFTUP;SendInput(1,&input,sizeof(input));injected_button=false; }
    if(child.hProcess) {
        EnumWindows([](HWND w,LPARAM pid)->BOOL { DWORD process{};GetWindowThreadProcessId(w,&process);
            if(process==static_cast<DWORD>(pid)) PostMessageW(w,WM_CLOSE,0,0);return TRUE;
        },child.dwProcessId);
        if(WaitForSingleObject(child.hProcess,3000)==WAIT_TIMEOUT) TerminateProcess(child.hProcess,99);
        CloseHandle(child.hProcess); CloseHandle(child.hThread); child={};
    }
    if(fixture) { DestroyWindow(fixture); fixture=nullptr; }
    SetCursorPos(original_pointer.x,original_pointer.y);
}
void check(bool value,const char* message) {
    if(!value) { std::cerr<<"FAIL: "<<message<<" (Win32 "<<GetLastError()<<")\n";cleanup();std::exit(1); }
    std::cout<<"PASS: "<<message<<'\n';
}
LRESULT CALLBACK procedure(HWND hwnd,UINT msg,WPARAM w,LPARAM l) {
    if(msg==WM_ERASEBKGND) {
        RECT r{};GetClientRect(hwnd,&r);auto brush=CreateSolidBrush(backdrop_color);
        FillRect(reinterpret_cast<HDC>(w),&r,brush);DeleteObject(brush);return 1;
    }
    if(msg==WM_LBUTTONDOWN) { ++clicks;return 0; }
    if(msg==WM_HOTKEY) { ++hotkeys;return 0; }
    return DefWindowProcW(hwnd,msg,w,l);
}
HWND launch(const std::wstring& executable,bool visible=true) {
    STARTUPINFOW info{}; info.cb=sizeof(info);
    auto cmd=L"\""+executable+L"\"";
    check(CreateProcessW(executable.c_str(),cmd.data(),nullptr,nullptr,FALSE,0,nullptr,nullptr,&info,&child)!=FALSE,"launch standalone executable");
    HWND result{};
    for(int i=0;i<300 && !result;++i) {
        struct Context { DWORD pid;HWND* result;bool visible; } context{child.dwProcessId,&result,visible};
        EnumWindows([](HWND hwnd,LPARAM data)->BOOL {
            auto context=reinterpret_cast<Context*>(data);DWORD process{};GetWindowThreadProcessId(hwnd,&process);
            wchar_t name[128]{};GetClassNameW(hwnd,name,128);
            if(process==context->pid && std::wstring(name)==L"InputOverlayDesktopWindow" && (IsWindowVisible(hwnd)||!context->visible)) { *context->result=hwnd;return FALSE; }
            return TRUE;
        },reinterpret_cast<LPARAM>(&context));
        pump(30);
        if(WaitForSingleObject(child.hProcess,0)==WAIT_OBJECT_0) break;
    }
    check(result!=nullptr,"native window created without OBS");return result;
}
RECT rect(HWND w) { RECT r{};GetWindowRect(w,&r);return r; }
void action(HWND hwnd,unsigned command) { SendMessageW(hwnd,WM_COMMAND,command,0);pump(100); }
HWND common_dialog(HWND owner) {
    HWND result{};
    struct Context {HWND owner;HWND* result;};Context context{owner,&result};
    for(int attempt=0;attempt<60&&!result;++attempt) {
        EnumThreadWindows(GetWindowThreadProcessId(owner,nullptr),[](HWND w,LPARAM data)->BOOL {
            auto context=reinterpret_cast<Context*>(data);wchar_t klass[64]{};GetClassNameW(w,klass,64);
            if(GetWindow(w,GW_OWNER)==context->owner&&std::wstring(klass)==L"#32770"&&IsWindowVisible(w)) {*context->result=w;return FALSE;}return TRUE;
        },reinterpret_cast<LPARAM>(&context));pump(25);
    }return result;
}
void mouse(DWORD flag) { INPUT input{};input.type=INPUT_MOUSE;input.mi.dwFlags=flag;check(SendInput(1,&input,sizeof(input))==1,"mouse input injected into test desktop"); injected_button=flag==MOUSEEVENTF_LEFTDOWN; }
void key(WORD vk,bool up=false) { INPUT input{};input.type=INPUT_KEYBOARD;input.ki.wVk=vk;
    input.ki.dwFlags=up?KEYEVENTF_KEYUP:0;check(SendInput(1,&input,sizeof(input))==1,"shortcut test input injected");injected_keys[vk]=!up; }
void toggle_hotkey() { key(VK_CONTROL);key(VK_MENU);key(VK_F8);key(VK_F8,true);key(VK_MENU,true);key(VK_CONTROL,true);pump(100); }
void field(HWND dialog,int id,const std::wstring& value) {
    // Use the control's real edit path across processes (not caption APIs).
    auto control=GetDlgItem(dialog,id);SendMessageW(control,EM_SETSEL,0,-1);
    for(auto character:value) SendMessageW(control,WM_CHAR,character,1);
}
void pointer(POINT point,DWORD button=0) {
    INPUT input{};input.type=INPUT_MOUSE;
    const auto x=GetSystemMetrics(SM_XVIRTUALSCREEN),y=GetSystemMetrics(SM_YVIRTUALSCREEN);
    input.mi.dx=static_cast<LONG>((double(point.x-x)+0.5)*65536/GetSystemMetrics(SM_CXVIRTUALSCREEN));
    input.mi.dy=static_cast<LONG>((double(point.y-y)+0.5)*65536/GetSystemMetrics(SM_CYVIRTUALSCREEN));
    input.mi.dwFlags=MOUSEEVENTF_MOVE|MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_VIRTUALDESK|button;
    check(SendInput(1,&input,sizeof(input))==1,"absolute gesture event injected");
    if(button==MOUSEEVENTF_LEFTDOWN) injected_button=true;
    if(button==MOUSEEVENTF_LEFTUP) injected_button=false;
}
void drag(POINT from,POINT to) {
    SetWindowPos(fixture,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
    SetWindowPos(overlay_window,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
    pointer(from);pump(30);
    check(WindowFromPoint(from)==overlay_window,"gesture starts on selected overlay");
    pointer(from,MOUSEEVENTF_LEFTDOWN);pump(60);
    GUITHREADINFO gui{};gui.cbSize=sizeof(gui);GetGUIThreadInfo(GetWindowThreadProcessId(overlay_window,nullptr),&gui);
    check(gui.hwndCapture==overlay_window,"edit gesture acquires pointer capture");
    pointer(to);pump(60);pointer(to,MOUSEEVENTF_LEFTUP);pump(100);
}
void stop(HWND hwnd) {
    action(hwnd,overlay::command::exit);
    check(WaitForSingleObject(child.hProcess,5000)==WAIT_OBJECT_0,"graceful exit");
    CloseHandle(child.hProcess);CloseHandle(child.hThread);child={};
}
void screenshot(HWND hwnd,const std::filesystem::path& path) {
    auto r=rect(hwnd);const int w=r.right-r.left,h=r.bottom-r.top;
    HDC screen=GetDC(nullptr),dc=CreateCompatibleDC(screen);
    BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=w;
    info.bmiHeader.biHeight=-h;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
    void* bits{};auto bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&bits,nullptr,0);
    check(bitmap!=nullptr,"screenshot surface allocated");
    auto previous=SelectObject(dc,bitmap);
    const auto ok=BitBlt(dc,0,0,w,h,screen,r.left,r.top,SRCCOPY|CAPTUREBLT);
    if(ok) { BITMAPFILEHEADER header{};header.bfType=0x4d42;header.bfOffBits=sizeof(header)+sizeof(BITMAPINFOHEADER);
        header.bfSize=header.bfOffBits+w*h*4;std::ofstream out(path,std::ios::binary);
        out.write(reinterpret_cast<const char*>(&header),sizeof(header));out.write(reinterpret_cast<const char*>(&info.bmiHeader),sizeof(info.bmiHeader));
        out.write(static_cast<const char*>(bits),w*h*4);
    }
    SelectObject(dc,previous);DeleteObject(bitmap);DeleteDC(dc);ReleaseDC(nullptr,screen);
}
}
int wmain(int argc,wchar_t** argv) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    GetCursorPos(&original_pointer);
    check(argc==2,"application path supplied");
    const auto profile=std::filesystem::absolute(std::filesystem::path(L"build")/(L"integration-profile-"+std::to_wstring(GetCurrentProcessId())));
    std::filesystem::create_directories(profile/L"InputOverlayDesktop");
    SetEnvironmentVariableW(L"LOCALAPPDATA",profile.c_str());
    const auto config=profile/L"InputOverlayDesktop/settings.ini";
    WritePrivateProfileStringW(L"window",L"hotkey_enabled",L"0",config.c_str());
    WNDCLASSW wc{};wc.lpfnWndProc=procedure;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"InputOverlayTestBackdrop";
    wc.hbrBackground=CreateSolidBrush(RGB(55,65,80));RegisterClassW(&wc);
    MONITORINFO monitor{};monitor.cbSize=sizeof(monitor);GetMonitorInfoW(MonitorFromPoint({0,0},MONITOR_DEFAULTTOPRIMARY),&monitor);
    const auto area=monitor.rcWork;
    fixture=CreateWindowExW(0,wc.lpszClassName,L"Input Overlay interaction test backdrop",WS_POPUP,
        area.left,area.top,area.right-area.left,area.bottom-area.top,nullptr,nullptr,wc.hInstance,nullptr);
    ShowWindow(fixture,SW_SHOW);SetForegroundWindow(fixture);pump(100);
    const std::wstring executable=std::filesystem::absolute(argv[1]).wstring();
    auto hwnd=launch(executable);overlay_window=hwnd;
    check((GetWindowLongPtrW(hwnd,GWL_EXSTYLE)&(WS_EX_TRANSPARENT|WS_EX_NOACTIVATE|WS_EX_TOPMOST))
        ==(WS_EX_TRANSPARENT|WS_EX_NOACTIVATE|WS_EX_TOPMOST),"locked startup styles and always-on-top");
    NOTIFYICONIDENTIFIER icon{};icon.cbSize=sizeof(icon);icon.hWnd=hwnd;icon.uID=1;RECT icon_rect{};
    check(SUCCEEDED(Shell_NotifyIconGetRect(&icon,&icon_rect)),"recovery tray icon registered");
    action(hwnd,overlay::command::edit);
    check(!(GetWindowLongPtrW(hwnd,GWL_EXSTYLE)&WS_EX_TRANSPARENT),"edit mode interactive");
    auto gap=rect(hwnd);
    check(WindowFromPoint({gap.left+MulDiv(112,gap.right-gap.left,338),gap.top+MulDiv(20,gap.bottom-gap.top,148)})==hwnd,
        "edit selection interior captures transparent key gaps");
    auto r=rect(hwnd);POINT inside{r.left+(r.right-r.left)/2,r.top+(r.bottom-r.top)/2};
    const LONG dx=area.right-r.right-24,dy=area.top-r.top+24;
    drag(inside,{inside.x+dx,inside.y+dy});r=rect(hwnd);
    if(std::abs(r.right-(area.right-24))>2 || std::abs(r.top-(area.top+24))>2)
        std::cerr<<"Move result "<<r.left<<","<<r.top<<","<<r.right<<","<<r.bottom
            <<" expected right/top "<<area.right-24<<","<<area.top+24<<'\n';
    check(std::abs(r.right-(area.right-24))<=2 && std::abs(r.top-(area.top+24))<=2,"move selection to top-right");
    const int dpi=GetDpiForWindow(hwnd);const int inset=MulDiv(4,dpi,96);
    // Pull the bottom-left corner outward; top-right remains anchored.
    const auto anchor=r;drag({r.left+inset,r.bottom-inset},{r.left+inset-100,r.bottom-inset+44});r=rect(hwnd);
    check(r.right==anchor.right && r.top==anchor.top && r.left<anchor.left,"corner resize keeps opposite anchor");
    check(std::abs(double(r.right-r.left)/(r.bottom-r.top)-338.0/148)<0.02,"keyboard and mouse retain proportional transform");
    action(hwnd,overlay::command::aspect); // unlock edge resizing
    const auto before_edge=r;
    drag({r.right-inset,(r.top+r.bottom)/2},{r.right-inset-40,(r.top+r.bottom)/2});r=rect(hwnd);
    if(r.bottom-r.top!=before_edge.bottom-before_edge.top || r.right-r.left!=before_edge.right-before_edge.left-40)
        std::cerr<<"Edge bounds before "<<before_edge.left<<","<<before_edge.top<<","<<before_edge.right<<","<<before_edge.bottom
            <<" after "<<r.left<<","<<r.top<<","<<r.right<<","<<r.bottom<<" window valid "<<IsWindow(hwnd)<<'\n';
    check(r.bottom-r.top==before_edge.bottom-before_edge.top && r.right-r.left==before_edge.right-before_edge.left-40,
        "unlocked edge supports single-axis resizing");
    key(VK_LSHIFT);pump(50);
    drag({r.right-inset,(r.top+r.bottom)/2},{r.right-inset+40,(r.top+r.bottom)/2});key(VK_LSHIFT,true);pump(50);r=rect(hwnd);
    check(std::abs(double(r.right-r.left)/(r.bottom-r.top)-338.0/148)<0.02,"Shift edge restores original aspect ratio");
    action(hwnd,overlay::command::aspect); // restore default aspect lock
    inside={r.left+MulDiv(90,r.right-r.left,338),r.top+MulDiv(74,r.bottom-r.top,148)};
    // Isolate from unrelated always-on-top desktop apps. First prove the opaque
    // key receives hit testing; then keep exactly the same window ordering while
    // locking and verify the backdrop receives a real click across processes.
    auto order=[&] {
        SetWindowPos(fixture,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
        SetWindowPos(hwnd,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);pump(50);
    };
    order();check(WindowFromPoint(inside)==hwnd,"opaque key intercepts test point in edit mode");
    screenshot(hwnd,profile/L"edit-mode.bmp");
    action(hwnd,overlay::command::lock);order();
    HDC desktop=GetDC(nullptr);const auto pixel=GetPixel(desktop,inside.x,inside.y);ReleaseDC(nullptr,desktop);
    check(pixel!=CLR_INVALID && pixel!=RGB(55,65,80),"locked painted key remains visibly above test backdrop");
    screenshot(hwnd,profile/L"locked-mode.bmp");
    // Verify alpha composition against known backgrounds, not just window styles.
    for(auto color : {RGB(255,255,255),RGB(0,0,0),RGB(55,65,80)}) {
        backdrop_color=color;RedrawWindow(fixture,nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_UPDATENOW);
        order();
        POINT gap_point{r.left+MulDiv(215,r.right-r.left,338),r.top+MulDiv(75,r.bottom-r.top,148)};
        auto dc=GetDC(nullptr);const auto gap_color=GetPixel(dc,gap_point.x,gap_point.y);
        const auto key_color=GetPixel(dc,inside.x,inside.y);ReleaseDC(nullptr,dc);
        check(gap_color==color,"locked keyboard-mouse gap is fully transparent over backdrop");
        check(key_color!=color && key_color!=CLR_INVALID,"key stays visibly distinct on light and dark backdrops");
    }
    if(WindowFromPoint(inside)!=fixture) {
        wchar_t target[128]{};GetClassNameW(WindowFromPoint(inside),target,128);
        std::wcerr<<L"Probe "<<inside.x<<L","<<inside.y<<L" rect "<<r.left<<L","<<r.top<<L","<<r.right<<L","<<r.bottom
            <<L" target "<<target<<L" fixture "<<fixture<<L" foreground "<<GetForegroundWindow()<<L'\n';
    }
    check(WindowFromPoint(inside)==fixture,"locked hit testing reaches separate-process backdrop");
    const auto before=clicks;SetCursorPos(inside.x,inside.y);pump(30);mouse(MOUSEEVENTF_LEFTDOWN);mouse(MOUSEEVENTF_LEFTUP);pump(100);
    check(clicks==before+1,"real locked click delivered underneath");
    check(GetForegroundWindow()==fixture,"locked overlay does not steal keyboard focus");
    const auto saved=rect(hwnd);stop(hwnd);hwnd=launch(executable);r=rect(hwnd);
    check(std::abs(r.left-saved.left)<=1 && std::abs(r.top-saved.top)<=1 && std::abs(r.right-saved.right)<=2 && std::abs(r.bottom-saved.bottom)<=2,
        "restart restores position and size");
    check((GetWindowLongPtrW(hwnd,GWL_EXSTYLE)&WS_EX_TRANSPARENT)!=0,"restart restores locked mode");
    action(hwnd,overlay::command::edit);stop(hwnd);hwnd=launch(executable);
    check(!(GetWindowLongPtrW(hwnd,GWL_EXSTYLE)&WS_EX_TRANSPARENT),"restart restores edit mode");
    SetWindowPos(hwnd,nullptr,-20000,-20000,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
    SendMessageW(hwnd,WM_DISPLAYCHANGE,0,0);pump(100);r=rect(hwnd);
    check(MonitorFromRect(&r,MONITOR_DEFAULTTONULL)!=nullptr,"monitor-change notification recovers offscreen HUD");
    action(hwnd,overlay::command::reset);r=rect(hwnd);
    check(r.left>=area.left && r.top>=area.top && r.right<=area.right && r.bottom<=area.bottom,"tray reset recovers accessible geometry");
    // Exercise real surface destruction/recreation repeatedly and check GDI ownership.
    const auto original=r;const auto gdi_before=GetGuiResources(child.hProcess,GR_GDIOBJECTS);
    for(int i=0;i<80;++i) {
        const auto width=original.right-original.left+(i%2)*90;
        const auto height=original.bottom-original.top+(i%2)*40;
        SetWindowPos(hwnd,nullptr,original.left,original.top,width,height,SWP_NOZORDER|SWP_NOACTIVATE);
        RedrawWindow(hwnd,nullptr,nullptr,RDW_INVALIDATE|RDW_UPDATENOW);
    }
    pump(100);const auto gdi_after=GetGuiResources(child.hProcess,GR_GDIOBJECTS);
    check(gdi_before>0 && gdi_after<=gdi_before+4,"80 surface resizes do not accumulate GDI handles");
    action(hwnd,overlay::command::reset);
    // Recreate the same lost-icon state that Explorer restart produces, without
    // terminating the user's Explorer or touching other applications' icons.
    NOTIFYICONDATAW removed{};removed.cbSize=sizeof(removed);removed.hWnd=hwnd;removed.uID=1;
    check(Shell_NotifyIconW(NIM_DELETE,&removed)!=FALSE,"remove only test overlay tray icon");
    SendMessageW(hwnd,RegisterWindowMessageW(L"TaskbarCreated"),0,0);pump(100);
    icon.hWnd=hwnd;
    check(SUCCEEDED(Shell_NotifyIconGetRect(&icon,&icon_rect)),"tray icon is re-added after Shell recovery notification");
    // Exercise the real Shell callback and popup, not just WM_COMMAND handlers.
    const POINT tray_point{(icon_rect.left+icon_rect.right)/2,(icon_rect.top+icon_rect.bottom)/2};
    RECT virtual_screen{GetSystemMetrics(SM_XVIRTUALSCREEN),GetSystemMetrics(SM_YVIRTUALSCREEN),
        GetSystemMetrics(SM_XVIRTUALSCREEN)+GetSystemMetrics(SM_CXVIRTUALSCREEN),
        GetSystemMetrics(SM_YVIRTUALSCREEN)+GetSystemMetrics(SM_CYVIRTUALSCREEN)};
    if(PtInRect(&virtual_screen,tray_point) && WindowFromPoint(tray_point)) {
        auto choose=[&](int index) {
            pointer(tray_point,MOUSEEVENTF_RIGHTDOWN);pointer(tray_point,MOUSEEVENTF_RIGHTUP);pump(150);
            GUITHREADINFO gui{};gui.cbSize=sizeof(gui);GetGUIThreadInfo(GetWindowThreadProcessId(hwnd,nullptr),&gui);
            if(gui.hwndMenuOwner!=hwnd) { key(VK_ESCAPE);key(VK_ESCAPE,true);pump(50);return false; }
            key(VK_HOME);key(VK_HOME,true);
            for(int i=0;i<index;++i) { key(VK_DOWN);key(VK_DOWN,true); }
            key(VK_RETURN);key(VK_RETURN,true);pump(100);return true;
        };
        if(choose(1)) {
            check((GetWindowLongPtrW(hwnd,GWL_EXSTYLE)&WS_EX_TRANSPARENT)!=0,"real tray popup Lock command activates click-through");
            check(choose(0),"reopen real tray popup");
            check(!(GetWindowLongPtrW(hwnd,GWL_EXSTYLE)&WS_EX_TRANSPARENT),"real tray popup Edit command activates selection");
        } else std::cout<<"NOT EXERCISED: tray icon is in a hidden/overflow area; popup requires manual opening\n";
    } else std::cout<<"NOT EXERCISED: tray icon has no visible desktop rectangle\n";
    std::vector<HMONITOR> monitors;
    EnumDisplayMonitors(nullptr,nullptr,[](HMONITOR m,HDC,LPRECT,LPARAM data)->BOOL {
        reinterpret_cast<std::vector<HMONITOR>*>(data)->push_back(m);return TRUE;
    },reinterpret_cast<LPARAM>(&monitors));
    const auto start_dpi=GetDpiForWindow(hwnd);const auto start_rect=rect(hwnd);
    const auto logical_w=MulDiv(start_rect.right-start_rect.left,96,start_dpi);
    const auto logical_h=MulDiv(start_rect.bottom-start_rect.top,96,start_dpi);
    for(auto monitor_handle : monitors) {
        MONITORINFO info{};info.cbSize=sizeof(info);GetMonitorInfoW(monitor_handle,&info);
        SetWindowPos(hwnd,nullptr,info.rcWork.left+32,info.rcWork.top+32,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
        pump(150);const auto actual=rect(hwnd);const auto actual_dpi=GetDpiForWindow(hwnd);
        check(MonitorFromWindow(hwnd,MONITOR_DEFAULTTONEAREST)==monitor_handle,"move onto available monitor");
        check(std::abs(MulDiv(actual.right-actual.left,96,actual_dpi)-logical_w)<=2
            && std::abs(MulDiv(actual.bottom-actual.top,96,actual_dpi)-logical_h)<=2,"monitor move preserves logical HUD size");
        std::cout<<"Monitor DPI: "<<actual_dpi<<'\n';
    }
    if(monitors.size()<2) std::cout<<"NOT EXERCISED: physical multi-monitor and mixed-DPI transitions (one connected monitor)\n";
    action(hwnd,overlay::command::reset);
    WritePrivateProfileStringW(L"window",L"hotkey_enabled",L"1",config.c_str());
    action(hwnd,overlay::command::reload);
    action(hwnd,overlay::command::lock);toggle_hotkey();
    check(!(GetWindowLongPtrW(hwnd,GWL_EXSTYLE)&WS_EX_TRANSPARENT),"real global shortcut enters edit mode");
    toggle_hotkey();
    check((GetWindowLongPtrW(hwnd,GWL_EXSTYLE)&WS_EX_TRANSPARENT)!=0,"real global shortcut locks overlay");
    stop(hwnd);
    check(RegisterHotKey(fixture,19,MOD_CONTROL|MOD_ALT|MOD_NOREPEAT,VK_F8)!=FALSE,"exit releases global shortcut");
    hwnd=launch(executable);const auto before_hotkeys=hotkeys;toggle_hotkey();
    check(hotkeys==before_hotkeys+1 && (GetWindowLongPtrW(hwnd,GWL_EXSTYLE)&WS_EX_TRANSPARENT),"shortcut conflict keeps existing owner and tray fallback");
    stop(hwnd);UnregisterHotKey(fixture,19);
    // Modeless settings apply/persistence and common native picker interactions.
    WritePrivateProfileStringW(L"window",L"hotkey_enabled",L"0",config.c_str());
    hwnd=launch(executable);action(hwnd,overlay::command::settings);pump(150);
    const HWND settings=GetLastActivePopup(hwnd);wchar_t klass[64]{};GetClassNameW(settings,klass,64);
    check(std::wstring(klass)==L"InputOverlaySettings","native settings window opens");
    using namespace overlay::settings_control;
    check(GetDpiForWindow(settings)==GetDpiForWindow(hwnd),"settings window uses current monitor DPI");
    PostMessageW(settings,WM_COMMAND,Pressed,0);pump(150);
    auto picker=common_dialog(settings);
    if(picker==settings||!GetDlgItem(picker,COLOR_RED)) {
        wchar_t popup_class[64]{};GetClassNameW(picker,popup_class,64);std::wcerr<<L"Picker class "<<popup_class<<L" handle "<<picker<<L'\n';
        EnumChildWindows(picker,[](HWND control,LPARAM)->BOOL {wchar_t klass[64]{};GetClassNameW(control,klass,64);std::wcerr<<L"Picker child "<<GetDlgCtrlID(control)<<L" "<<klass<<L'\n';return TRUE;},0);
    }
    check(picker!=settings&&GetDlgItem(picker,COLOR_RED),"native color picker opens");
    for(auto pair:{std::pair{COLOR_RED,17},std::pair{COLOR_GREEN,34},std::pair{COLOR_BLUE,51}}) {
        SendMessageW(picker,WM_NEXTDLGCTL,reinterpret_cast<WPARAM>(GetDlgItem(picker,pair.first)),TRUE);
        const auto control=GetDlgItem(picker,pair.first);SendMessageW(control,EM_SETSEL,0,-1);
        for(auto digit:std::to_wstring(pair.second)) SendMessageW(control,WM_CHAR,digit,1);
    }
    SendMessageW(GetDlgItem(picker,IDOK),BM_CLICK,0,0);pump(150);
    wchar_t button[64]{};GetDlgItemTextW(settings,Pressed,button,64);
    for(int i=0;i<80&&std::wstring(button).find(L"112233")==std::wstring::npos;++i) {PostMessageW(settings,WM_NULL,0,0);pump(25);GetDlgItemTextW(settings,Pressed,button,64);}
    check(std::wstring(button).find(L"112233")!=std::wstring::npos,"color picker selection updates draft");
    PostMessageW(settings,WM_COMMAND,FontPicker,0);pump(150);picker=common_dialog(settings);
    check(picker!=settings&&GetDlgItem(picker,cmb1),"native font picker opens");
    SendMessageW(picker,WM_COMMAND,IDCANCEL,0);pump(80);
    field(settings,Family,L"Consolas");field(settings,FontSize,L"18");
    field(settings,Thickness,L"2.0");field(settings,Background,L"50");
    field(settings,Opacity,L"80");field(settings,Scale,L"125");field(settings,Spacing,L"56");
    auto select_page=[&](int page) {auto tab=GetDlgItem(settings,Tab);SendMessageW(tab,WM_KEYDOWN,VK_HOME,0);
        for(int i=0;i<page;++i) SendMessageW(tab,WM_KEYDOWN,VK_RIGHT,0);pump(80);};
    select_page(1);check(IsWindowVisible(GetDlgItem(settings,Preset)),"layout tab controls visible");
    field(settings,KeyX,L"88");field(settings,KeyY,L"18");CheckDlgButton(settings,KeyEnabled,BST_UNCHECKED);
    CheckDlgButton(settings,Mouse,BST_UNCHECKED);
    select_page(2);CheckDlgButton(settings,Topmost,BST_UNCHECKED);CheckDlgButton(settings,Edit,BST_UNCHECKED);
    SendMessageW(settings,WM_COMMAND,Apply,0);pump(120);
    check(GetPrivateProfileIntW(L"window",L"pressed_color",0,config.c_str())==0x112233,"settings Apply saves chosen color");
    check(GetPrivateProfileIntW(L"window",L"key0_x",0,config.c_str())==88
        &&GetPrivateProfileIntW(L"window",L"key0_enabled",1,config.c_str())==0,"custom key position and visibility saved");
    check(GetPrivateProfileIntW(L"window",L"font_size",0,config.c_str())==18
        &&GetPrivateProfileIntW(L"window",L"overlay_opacity",0,config.c_str())==80
        &&GetPrivateProfileIntW(L"window",L"border_tenths",0,config.c_str())==20,"appearance edits applied from native controls");
    check(!(GetWindowLongPtrW(hwnd,GWL_EXSTYLE)&WS_EX_TOPMOST)
        &&(GetWindowLongPtrW(hwnd,GWL_EXSTYLE)&WS_EX_TRANSPARENT),"settings topmost toggle preserves locked click-through");
    select_page(0);SetWindowPos(settings,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
    RedrawWindow(settings,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_UPDATENOW);pump(150);
    screenshot(settings,profile/L"settings-window.bmp");
    select_page(1);SendMessageW(settings,WM_COMMAND,ResetLayout,0);SendMessageW(settings,WM_COMMAND,Apply,0);pump(100);
    check(GetPrivateProfileIntW(L"window",L"key0_x",0,config.c_str())==70
        &&GetPrivateProfileIntW(L"window",L"show_mouse",0,config.c_str())==1,"native layout reset restores arrangement and mouse");
    SendMessageW(settings,WM_COMMAND,ResetGeometry,0);SendMessageW(settings,WM_COMMAND,Apply,0);pump(80);
    SendMessageW(settings,WM_COMMAND,Apply,0);pump(80);
    check(GetPrivateProfileIntW(L"window",L"scale",0,config.c_str())==100,"settings position/size reset retains 100 percent on repeated Apply");
    SendMessageW(settings,WM_CLOSE,0,0);stop(hwnd);hwnd=launch(executable);
    check(!(GetWindowLongPtrW(hwnd,GWL_EXSTYLE)&WS_EX_TOPMOST),"custom behavior survives restart");
    stop(hwnd);
    WritePrivateProfileStringW(L"window",L"launch_tray",L"1",config.c_str());
    hwnd=launch(executable,false);pump(150);
    check(!IsWindowVisible(hwnd),"launch-to-tray keeps HUD hidden");
    action(hwnd,overlay::command::edit);check(IsWindowVisible(hwnd),"tray Edit reveals hidden HUD");
    stop(hwnd);cleanup();
    std::wcout<<L"Windows integration passed; isolated profile and screenshots: "<<profile.wstring()<<L'\n';
}
