// SPDX-License-Identifier: GPL-2.0-only
#include "renderer.hpp"
#include <string>
namespace overlay {
HRESULT Renderer::initialize() {
    auto hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, factory_.GetAddressOf());
    if (FAILED(hr)) return hr;
    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(text_factory_.GetAddressOf()));
    if (FAILED(hr)) return hr;
    return text_factory_->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 18.0f, L"en-us", format_.GetAddressOf());
}
void Renderer::resize(UINT width, UINT height) {
    if (target_ && width && height && FAILED(target_->Resize(D2D1::SizeU(width, height)))) {
        brush_.Reset(); target_.Reset();
    }
}
HRESULT Renderer::draw(HWND window, const InputState& state) {
    if (!target_) {
        RECT rect{}; GetClientRect(window, &rect);
        auto hr = factory_->CreateHwndRenderTarget(D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(window, D2D1::SizeU(rect.right, rect.bottom)), target_.GetAddressOf());
        if (FAILED(hr)) return hr;
        hr = target_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), brush_.GetAddressOf());
        if (FAILED(hr)) { target_.Reset(); return hr; }
    }
    target_->BeginDraw();
    target_->Clear(D2D1::ColorF(0.06f, 0.08f, 0.12f));
    auto text = [&](const std::wstring& value, float x, float y, float width) {
        brush_->SetColor(D2D1::ColorF(D2D1::ColorF::White));
        target_->DrawText(value.c_str(), static_cast<UINT32>(value.size()), format_.Get(),
            D2D1::RectF(x, y, x + width, y + 40), brush_.Get());
    };
    std::wstring keys = L"Keys: ";
    for (unsigned key = 0; key < state.keys.size(); ++key) {
        if (!state.keys[key]) continue;
        wchar_t name[64]{};
        UINT scan = MapVirtualKeyW(key, MAPVK_VK_TO_VSC_EX);
        LONG bits = static_cast<LONG>((scan & 0xff) << 16);
        if (scan & 0xff00) bits |= 1L << 24;
        if (GetKeyNameTextW(bits, name, 64)) keys += std::wstring(name) + L"  ";
        else keys += L"VK " + std::to_wstring(key) + L"  ";
    }
    text(L"Input Overlay Desktop", 18, 12, 650);
    text(keys, 18, 48, target_->GetSize().width - 36);
    constexpr const wchar_t* labels[]{L"Left", L"Right", L"Middle", L"X1", L"X2"};
    for (unsigned i = 0; i < 5; ++i) {
        float x = 18.0f + i * 112.0f;
        brush_->SetColor(state.buttons[i] ? D2D1::ColorF(0.1f, 0.65f, 0.4f) : D2D1::ColorF(0.18f, 0.22f, 0.3f));
        target_->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(x, 98, x + 100, 143), 7, 7), brush_.Get());
        text(labels[i], x + 10, 108, 90);
    }
    text(L"Mouse: " + std::to_wstring(state.mouse_x) + L", " + std::to_wstring(state.mouse_y)
        + L"    Wheel: " + std::to_wstring(state.wheel), 18, 155, 650);
    auto hr = target_->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) { brush_.Reset(); target_.Reset(); return S_OK; }
    return hr;
}
}
