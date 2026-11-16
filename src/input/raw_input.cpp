// SPDX-License-Identifier: GPL-2.0-only
#include "raw_input.hpp"
#include <vector>

namespace overlay {
bool RawInput::attach(HWND window) {
    detach();
    // No NOLEGACY/CAPTUREMOUSE flags: ordinary application input is untouched.
    RAWINPUTDEVICE devices[]{{1, 6, RIDEV_INPUTSINK | RIDEV_DEVNOTIFY, window},
        {1, 2, RIDEV_INPUTSINK | RIDEV_DEVNOTIFY, window}};
    if (!RegisterRawInputDevices(devices, 2, sizeof(RAWINPUTDEVICE))) return false;
    window_ = window;
    return true;
}
void RawInput::detach() {
    if (!window_) return;
    RAWINPUTDEVICE devices[]{{1, 6, RIDEV_REMOVE, nullptr}, {1, 2, RIDEV_REMOVE, nullptr}};
    RegisterRawInputDevices(devices, 2, sizeof(RAWINPUTDEVICE));
    window_ = nullptr;
}
void RawInput::handle(HRAWINPUT handle, InputState& state) const {
    UINT size{};
    if (!window_) return;
    if (GetRawInputData(handle, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER)) != 0
        || size < sizeof(RAWINPUTHEADER)) { state.clear(); return; }
    // operator new supplies alignment suitable for RAWINPUT.
    std::vector<std::byte> storage(size);
    const auto bytes = GetRawInputData(handle, RID_INPUT, storage.data(), &size, sizeof(RAWINPUTHEADER));
    if (bytes == UINT(-1) || bytes != storage.size()) { state.clear(); return; }
    const auto& data = *reinterpret_cast<const RAWINPUT*>(storage.data());
    const auto device = reinterpret_cast<InputState::Device>(data.header.hDevice);
    // Reject malformed packets before accessing their payloads.
    if (data.header.dwSize != bytes) { state.clear(); return; }
    if (data.header.dwType == RIM_TYPEKEYBOARD) {
        if (bytes < offsetof(RAWINPUT, data) + sizeof(RAWKEYBOARD)) { state.clear(); return; }
        const auto& key = data.data.keyboard;
        constexpr USHORT keyboard_overrun_make_code = 0xff;
        if (key.MakeCode == keyboard_overrun_make_code) { state.remove_device(device); return; }
        if (!key.MakeCode || key.VKey == 255 || (key.Flags & RI_KEY_E1)) return;
        const unsigned code = key.MakeCode | ((key.Flags & RI_KEY_E0) ? 0x80u : 0u);
        state.apply({EventType::Key, code, !(key.Flags & RI_KEY_BREAK)}, device);
    } else if (data.header.dwType == RIM_TYPEMOUSE) {
        if (bytes < offsetof(RAWINPUT, data) + sizeof(RAWMOUSE)) { state.clear(); return; }
        const auto& mouse = data.data.mouse;
        POINT point{};
        if (GetCursorPos(&point)) state.apply({EventType::Motion, 0, false, point.x, point.y});
        constexpr USHORT down[]{RI_MOUSE_LEFT_BUTTON_DOWN, RI_MOUSE_RIGHT_BUTTON_DOWN,
            RI_MOUSE_MIDDLE_BUTTON_DOWN, RI_MOUSE_BUTTON_4_DOWN, RI_MOUSE_BUTTON_5_DOWN};
        constexpr USHORT up[]{RI_MOUSE_LEFT_BUTTON_UP, RI_MOUSE_RIGHT_BUTTON_UP,
            RI_MOUSE_MIDDLE_BUTTON_UP, RI_MOUSE_BUTTON_4_UP, RI_MOUSE_BUTTON_5_UP};
        for (unsigned i = 0; i < 5; ++i) {
            if (mouse.usButtonFlags & down[i]) state.apply({EventType::Button, i, true}, device);
            if (mouse.usButtonFlags & up[i]) state.apply({EventType::Button, i, false}, device);
        }
        if (mouse.usButtonFlags & RI_MOUSE_WHEEL)
            state.apply({EventType::Wheel, 0, false, 0, 0, static_cast<SHORT>(mouse.usButtonData)});
    }
}
void RawInput::recover(InputState& state) const {
    if (!window_) return;
    std::array<bool, 256> keys{};
    std::array<bool, 5> buttons{};
    // Map the physical locations through the foreground thread's layout for the
    // release-only check. Raw Input remains the authority for all presses.
    const auto thread = GetWindowThreadProcessId(GetForegroundWindow(), nullptr);
    const auto layout = GetKeyboardLayout(thread);
    for (auto code : tracked_keys) {
        const UINT scan = (code & 0x80) ? (0xe000 | (code & 0x7f)) : code;
        const auto vk = MapVirtualKeyExW(scan, MAPVK_VSC_TO_VK_EX, layout);
        keys[code] = vk && (GetAsyncKeyState(static_cast<int>(vk)) & 0x8000);
    }
    constexpr int vks[]{VK_LBUTTON, VK_RBUTTON, VK_MBUTTON, VK_XBUTTON1, VK_XBUTTON2};
    for (unsigned i = 0; i < 5; ++i) buttons[i] = (GetAsyncKeyState(vks[i]) & 0x8000) != 0;
    state.reconcile(keys, buttons);
}
}
