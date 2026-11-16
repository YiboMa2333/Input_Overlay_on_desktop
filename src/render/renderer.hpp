// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include "com_ptr.hpp"
#include "input/input_state.hpp"
namespace overlay {
class Renderer {
    ComPtr<ID2D1Factory> factory_;
    ComPtr<IDWriteFactory> text_factory_;
    ComPtr<IDWriteTextFormat> format_;
    ComPtr<ID2D1DCRenderTarget> target_;
    ComPtr<ID2D1SolidColorBrush> brush_;
    ComPtr<ID2D1PathGeometry> mouse_, left_, right_;
    HDC dc_{};
    HBITMAP bitmap_{};
    HGDIOBJ previous_{};
    UINT width_{}, height_{};
    void release_surface();
    HRESULT surface(UINT width, UINT height);
public:
    Renderer() = default;
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    ~Renderer() { release_surface(); }
    HRESULT initialize();
    HRESULT draw(HWND window, const InputState& state, bool editing = false, bool resizing = false);
    void resize(UINT width, UINT height);
};
}
