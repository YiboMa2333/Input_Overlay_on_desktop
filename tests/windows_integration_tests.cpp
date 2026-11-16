// SPDX-License-Identifier: GPL-2.0-only
// Interactive test: opens a dedicated backdrop, moves the pointer, and launches
// only this test's child overlay with an isolated LOCALAPPDATA configuration.
#include <windows.h>
#include <shellapi.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <cstdlib>
#include <array>
#include <utility>
#include "desktop/commands.hpp"
namespace {
int clicks{}, hotkeys{};
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
    if(msg==WM_LBUTTONDOWN) { ++clicks;return 0; }
    if(msg==WM_HOTKEY) { ++hotkeys;return 0; }
    return DefWindowProcW(hwnd,msg,w,l);
}
HWND launch(const std::wstring& executable) {
    STARTUPINFOW info{}; info.cb=sizeof(info);
    auto cmd=L"\""+executable+L"\"";
    check(CreateProcessW(executable.c_str(),cmd.data(),nullptr,nullptr,FALSE,0,nullptr,nullptr,&info,&child)!=FALSE,"launch standalone executable");
    HWND result{};
    for(int i=0;i<300 && !result;++i) {
        std::pair<DWORD,HWND*> context{child.dwProcessId,&result};
        EnumWindows([](HWND hwnd,LPARAM data)->BOOL {
            auto pair=reinterpret_cast<std::pair<DWORD,HWND*>*>(data);DWORD process{};GetWindowThreadProcessId(hwnd,&process);
            wchar_t name[128]{};GetClassNameW(hwnd,name,128);
            if(process==pair->first && std::wstring(name)==L"InputOverlayDesktopWindow" && IsWindowVisible(hwnd)) { *pair->second=hwnd;return FALSE; }
            return TRUE;
        },reinterpret_cast<LPARAM>(&context));
        pump(30);
        if(WaitForSingleObject(child.hProcess,0)==WAIT_OBJECT_0) break;
    }
    check(result!=nullptr,"native window becomes visible without OBS");return result;
}
RECT rect(HWND w) { RECT r{};GetWindowRect(w,&r);return r; }
void action(HWND hwnd,unsigned command) { SendMessageW(hwnd,WM_COMMAND,command,0);pump(100); }
void mouse(DWORD flag) { INPUT input{};input.type=INPUT_MOUSE;input.mi.dwFlags=flag;check(SendInput(1,&input,sizeof(input))==1,"mouse input injected into test desktop"); injected_button=flag==MOUSEEVENTF_LEFTDOWN; }
void key(WORD vk,bool up=false) { INPUT input{};input.type=INPUT_KEYBOARD;input.ki.wVk=vk;
    input.ki.dwFlags=up?KEYEVENTF_KEYUP:0;check(SendInput(1,&input,sizeof(input))==1,"shortcut test input injected");injected_keys[vk]=!up; }
void toggle_hotkey() { key(VK_CONTROL);key(VK_MENU);key(VK_F8);key(VK_F8,true);key(VK_MENU,true);key(VK_CONTROL,true);pump(100); }
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
    stop(hwnd);UnregisterHotKey(fixture,19);cleanup();
    std::wcout<<L"Windows integration passed; isolated profile and screenshots: "<<profile.wstring()<<L'\n';
}
