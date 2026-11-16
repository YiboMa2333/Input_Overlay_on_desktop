// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>
#include "input/input_state.hpp"
namespace overlay {
class Renderer {
    Microsoft::WRL::ComPtr<ID2D1Factory> factory_;
    Microsoft::WRL::ComPtr<IDWriteFactory> text_factory_;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> format_;
    Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> target_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush_;
public:
    HRESULT initialize();
    HRESULT draw(HWND window, const InputState& state);
    void resize(UINT width, UINT height);
};
}
