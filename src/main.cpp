// SPDX-License-Identifier: GPL-2.0-only
#include "desktop/window.hpp"
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    overlay::Window application;
    int count{};auto args=CommandLineToArgvW(GetCommandLineW(),&count);bool tray=false;
    if(args) {for(int i=1;i<count;++i) if(std::wstring(args[i])==L"--tray") tray=true;LocalFree(args);}
    return application.run(instance, show,tray);
}
