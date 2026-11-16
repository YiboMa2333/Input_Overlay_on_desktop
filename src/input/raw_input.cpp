// SPDX-License-Identifier: GPL-2.0-only
#include "raw_input.hpp"
#include <vector>

namespace overlay {
bool RawInput::attach(HWND window) {
    RAWINPUTDEVICE devices[]{{1, 6, RIDEV_INPUTSINK, window}, {1, 2, RIDEV_INPUTSINK, window}};
    return RegisterRawInputDevices(devices, 2, sizeof(RAWINPUTDEVICE)) != FALSE;
}
void RawInput::detach() {
    RAWINPUTDEVICE devices[]{{1, 6, RIDEV_REMOVE, nullptr}, {1, 2, RIDEV_REMOVE, nullptr}};
    RegisterRawInputDevices(devices, 2, sizeof(RAWINPUTDEVICE));
}
void RawInput::handle(HRAWINPUT handle, InputState& state) const {
    UINT size{};
    if (GetRawInputData(handle, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER)) != 0 || !size) return;
    // operator new supplies alignment suitable for RAWINPUT.
    std::vector<std::byte> storage(size);
    if (GetRawInputData(handle, RID_INPUT, storage.data(), &size, sizeof(RAWINPUTHEADER)) == UINT(-1)) return;
    const auto& data = *reinterpret_cast<const RAWINPUT*>(storage.data());
    if (data.header.dwType == RIM_TYPEKEYBOARD) {
        const auto& key = data.data.keyboard;
        unsigned code = key.VKey;
        if (code == 255) return;
        if (code == VK_SHIFT) code = MapVirtualKeyW(key.MakeCode, MAPVK_VSC_TO_VK_EX);
        if (code == VK_CONTROL) code = (key.Flags & RI_KEY_E0) ? VK_RCONTROL : VK_LCONTROL;
        if (code == VK_MENU) code = (key.Flags & RI_KEY_E0) ? VK_RMENU : VK_LMENU;
        state.apply({EventType::Key, code, !(key.Flags & RI_KEY_BREAK)});
    } else if (data.header.dwType == RIM_TYPEMOUSE) {
        const auto& mouse = data.data.mouse;
        POINT point{};
        if (GetCursorPos(&point)) state.apply({EventType::Motion, 0, false, point.x, point.y});
        constexpr USHORT down[]{RI_MOUSE_LEFT_BUTTON_DOWN, RI_MOUSE_RIGHT_BUTTON_DOWN,
            RI_MOUSE_MIDDLE_BUTTON_DOWN, RI_MOUSE_BUTTON_4_DOWN, RI_MOUSE_BUTTON_5_DOWN};
        constexpr USHORT up[]{RI_MOUSE_LEFT_BUTTON_UP, RI_MOUSE_RIGHT_BUTTON_UP,
            RI_MOUSE_MIDDLE_BUTTON_UP, RI_MOUSE_BUTTON_4_UP, RI_MOUSE_BUTTON_5_UP};
        for (unsigned i = 0; i < 5; ++i) {
            if (mouse.usButtonFlags & down[i]) state.apply({EventType::Button, i, true});
            if (mouse.usButtonFlags & up[i]) state.apply({EventType::Button, i, false});
        }
        if (mouse.usButtonFlags & RI_MOUSE_WHEEL)
            state.apply({EventType::Wheel, 0, false, 0, 0, static_cast<SHORT>(mouse.usButtonData)});
    }
}
}
