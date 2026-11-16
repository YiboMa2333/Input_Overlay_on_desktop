// SPDX-License-Identifier: GPL-2.0-only
#include "desktop/window.hpp"
#include "desktop/instance.hpp"
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    int count{};auto args=CommandLineToArgvW(GetCommandLineW(),&count);bool tray=false;
    if(args) {for(int i=1;i<count;++i) if(std::wstring(args[i])==L"--tray") tray=true;LocalFree(args);}
    overlay::SingleInstance guard;
    if(!guard.valid()) {MessageBoxW(nullptr,L"Cannot create the per-user instance guard.",L"Input Overlay Desktop",MB_ICONERROR);return 1;}
    if(guard.existing()) {
        if(!tray) guard.activate();
        return 0;
    }
    overlay::Window application;
    return application.run(instance,show,tray,guard.identity);
}
