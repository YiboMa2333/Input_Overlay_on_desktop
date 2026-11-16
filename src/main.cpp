// SPDX-License-Identifier: GPL-2.0-only
#include "desktop/window.hpp"
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    SetProcessDPIAware();
    overlay::Window application;
    return application.run(instance, show);
}
