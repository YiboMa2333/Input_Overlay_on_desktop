// SPDX-License-Identifier: GPL-2.0-only
#include "desktop/window.hpp"
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    overlay::Window application;
    return application.run(instance, show);
}
