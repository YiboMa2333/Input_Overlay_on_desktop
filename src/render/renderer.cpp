// SPDX-License-Identifier: GPL-2.0-only
#include "renderer.hpp"
#include "hud_layout.hpp"
#include "desktop/geometry.hpp"
#include <cwchar>
namespace overlay {
namespace {
const auto ink = D2D1::ColorF(0xF2F0E7);
const auto edge = D2D1::ColorF(0x080A0D, 0.85f);
}
void Renderer::release_surface() {
    brush_.Reset(); target_.Reset();
    if (dc_ && previous_) SelectObject(dc_, previous_);
    if (bitmap_) DeleteObject(bitmap_);
    if (dc_) DeleteDC(dc_);
    dc_ = nullptr; bitmap_ = nullptr; previous_ = nullptr;
    width_ = height_ = 0;
}
HRESULT Renderer::initialize() {
    auto hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, factory_.GetAddressOf());
    if (FAILED(hr)) return hr;
    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(text_factory_.GetAddressOf()));
    if (FAILED(hr)) return hr;
    hr = text_factory_->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_SEMI_BOLD,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 13.0f, L"en-us", format_.GetAddressOf());
    if (FAILED(hr)) return hr;
    format_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    format_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    auto geometry = [&](ComPtr<ID2D1PathGeometry>& path, int region) -> HRESULT {
        auto result = factory_->CreatePathGeometry(path.GetAddressOf());
        if (FAILED(result)) return result;
        ComPtr<ID2D1GeometrySink> sink;
        result = path->Open(sink.GetAddressOf());
        if (FAILED(result)) return result;
        sink->BeginFigure({278, 8}, D2D1_FIGURE_BEGIN_FILLED);
        if (region == 1) {
            sink->AddBezier(D2D1::BezierSegment({251, 8}, {234, 27}, {234, 58}));
            sink->AddLine({234, 66}); sink->AddLine({278, 66});
        } else {
            sink->AddBezier(D2D1::BezierSegment({305, 8}, {322, 27}, {322, 58}));
            if (region == 2) { sink->AddLine({322, 66}); sink->AddLine({278, 66}); }
            else {
                sink->AddLine({322, 96});
                sink->AddBezier(D2D1::BezierSegment({322, 124}, {304, 140}, {278, 140}));
                sink->AddBezier(D2D1::BezierSegment({252, 140}, {234, 124}, {234, 96}));
                sink->AddLine({234, 58});
                sink->AddBezier(D2D1::BezierSegment({234, 27}, {251, 8}, {278, 8}));
            }
        }
        sink->EndFigure(D2D1_FIGURE_END_CLOSED);
        return sink->Close();
    };
    hr = geometry(mouse_, 0); if (FAILED(hr)) return hr;
    hr = geometry(left_, 1); if (FAILED(hr)) return hr;
    return geometry(right_, 2);
}
HRESULT Renderer::surface(UINT width, UINT height) {
    if (dc_ && width == width_ && height == height_) return S_OK;
    release_surface();
    dc_ = CreateCompatibleDC(nullptr);
    if (!dc_) return E_FAIL;
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = static_cast<LONG>(width);
    info.bmiHeader.biHeight = -static_cast<LONG>(height);
    info.bmiHeader.biPlanes = 1; info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    void* pixels{};
    bitmap_ = CreateDIBSection(dc_, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
    if (!bitmap_) { release_surface(); return E_FAIL; }
    previous_ = SelectObject(dc_, bitmap_);
    if (!previous_ || previous_ == HGDI_ERROR) { previous_ = nullptr; release_surface(); return E_FAIL; }
    auto properties = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_SOFTWARE,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 96, 96);
    auto hr = factory_->CreateDCRenderTarget(&properties, target_.GetAddressOf());
    if (FAILED(hr)) { release_surface(); return hr; }
    RECT bounds{0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
    hr = target_->BindDC(dc_, &bounds);
    if (SUCCEEDED(hr)) hr = target_->CreateSolidColorBrush(ink, brush_.GetAddressOf());
    if (FAILED(hr)) { release_surface(); return hr; }
    target_->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
    width_ = width; height_ = height;
    return S_OK;
}
void Renderer::resize(UINT width, UINT height) {
    if (width && height && (width != width_ || height != height_)) release_surface();
}
HRESULT Renderer::configure(const Settings& settings) {
    if(settings.font!=preferences_.font || settings.font_size!=preferences_.font_size) {
        ComPtr<IDWriteTextFormat> next;
        const auto hr=text_factory_->CreateTextFormat(settings.font.c_str(),nullptr,DWRITE_FONT_WEIGHT_SEMI_BOLD,
            DWRITE_FONT_STYLE_NORMAL,DWRITE_FONT_STRETCH_NORMAL,static_cast<float>(settings.font_size),L"en-us",next.GetAddressOf());
        if(FAILED(hr)) return hr;
        next->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);next->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        format_=std::move(next);
    }
    preferences_=settings;return S_OK;
}
HRESULT Renderer::draw(HWND window, const InputState& state, bool editing, bool resizing) {
    RECT bounds{}; GetClientRect(window, &bounds);
    if (bounds.right <= 0 || bounds.bottom <= 0) return S_OK;
    auto hr = surface(bounds.right, bounds.bottom);
    if (FAILED(hr)) return hr;
    target_->BeginDraw();
    target_->SetTransform(D2D1::Matrix3x2F::Identity());
    target_->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
    if (editing) {
        // Nonzero alpha makes the entire edit rectangle receive mouse messages,
        // including gaps. Locked mode never draws this catch surface.
        brush_->SetColor(D2D1::ColorF(0, 0, 0, 1.0f/255));
        target_->FillRectangle(D2D1::RectF(0, 0, static_cast<float>(width_), static_cast<float>(height_)), brush_.Get());
    }
    const auto extent=hud::extent(preferences_);
    const auto transform=D2D1::Matrix3x2F::Scale(width_/extent.width,height_/extent.height);
    const auto ink=D2D1::ColorF(preferences_.border_color);
    const auto yellow=D2D1::ColorF(preferences_.pressed_color);
    const auto dark=D2D1::ColorF(0x101317,preferences_.background_opacity/100.0f);
    const float thickness=preferences_.border_tenths/10.0f;
    const unsigned c=preferences_.pressed_color;
    const auto pressed_text=(((c>>16)&255)*299+((c>>8)&255)*587+(c&255)*114)>140000
        ? D2D1::ColorF(0x151713) : D2D1::ColorF(0xFFFFFF);
    target_->SetTransform(transform);
    auto rounded = [&](D2D1_ROUNDED_RECT shape, bool pressed) {
        brush_->SetColor(pressed ? yellow : dark);
        target_->FillRoundedRectangle(shape, brush_.Get());
        brush_->SetColor(edge); target_->DrawRoundedRectangle(shape, brush_.Get(), thickness+2.4f);
        brush_->SetColor(ink); target_->DrawRoundedRectangle(shape, brush_.Get(), thickness);
    };
    for (unsigned i=0;i<hud::keys.size();++i) {
        const auto& key=hud::keys[i];const auto& preference=preferences_.keys[i];
        if(!preference.enabled) continue;
        const bool pressed = key.pressed(state);
        const auto rect = D2D1::RectF(static_cast<float>(preference.x),static_cast<float>(preference.y),preference.x+key.width,preference.y+key.height);
        rounded(D2D1::RoundedRect(rect, 3.5f, 3.5f), pressed);
        brush_->SetColor(pressed ? pressed_text : ink);
        target_->DrawText(key.label, static_cast<UINT32>(std::wcslen(key.label)), format_.Get(),
            rect, brush_.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
    }
    if(preferences_.show_mouse) {
    target_->SetTransform(D2D1::Matrix3x2F::Translation(extent.mouse_x-234,0)*transform);
    brush_->SetColor(dark); target_->FillGeometry(mouse_.Get(), brush_.Get());
    brush_->SetColor(yellow);
    if (state.buttons[0]) target_->FillGeometry(left_.Get(), brush_.Get());
    if (state.buttons[1]) target_->FillGeometry(right_.Get(), brush_.Get());
    auto line = [&](D2D1_POINT_2F a, D2D1_POINT_2F b) {
        brush_->SetColor(edge); target_->DrawLine(a, b, brush_.Get(), thickness+2.4f);
        brush_->SetColor(ink); target_->DrawLine(a, b, brush_.Get(), thickness);
    };
    brush_->SetColor(edge); target_->DrawGeometry(mouse_.Get(), brush_.Get(), thickness+2.4f);
    brush_->SetColor(ink); target_->DrawGeometry(mouse_.Get(), brush_.Get(), thickness);
    line({234, 66}, {322, 66});
    line({278, 9}, {278, 24}); line({278, 57}, {278, 66});
    // Neutral wheel backing keeps left/right fills out of the middle region.
    brush_->SetColor(D2D1::ColorF(0x101317));
    target_->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(271, 25, 285, 56), 6, 6), brush_.Get());
    rounded(D2D1::RoundedRect(D2D1::RectF(271, 25, 285, 56), 6, 6), state.buttons[2]);
    for (float y : {34.0f, 40.0f, 46.0f}) {
        brush_->SetColor(state.buttons[2] ? pressed_text : ink);
        target_->DrawLine({275, y}, {281, y}, brush_.Get(), 1.0f);
    }
    if (state.wheel) {
        const float y = state.wheel > 0 ? 17.0f : 94.0f;
        const float tip = state.wheel > 0 ? y - 4 : y + 4;
        brush_->SetColor(edge);
        target_->DrawLine({272, y}, {278, tip}, brush_.Get(), 4);
        target_->DrawLine({278, tip}, {284, y}, brush_.Get(), 4);
        brush_->SetColor(yellow);
        target_->DrawLine({272, y}, {278, tip}, brush_.Get(), 2);
        target_->DrawLine({278, tip}, {284, y}, brush_.Get(), 2);
    }
    }
    if (editing) {
        target_->SetTransform(D2D1::Matrix3x2F::Identity());
        const float dpi = GetDpiForWindow(window)/96.0f;
        const float inset = 4*dpi;
        const auto boundary = D2D1::RectF(inset, inset, width_-inset, height_-inset);
        brush_->SetColor(edge); target_->DrawRectangle(boundary, brush_.Get(), 3*dpi);
        brush_->SetColor(D2D1::ColorF(0x83B9FF)); target_->DrawRectangle(boundary, brush_.Get(), dpi);
        for (const auto center : placement::handles(width_, height_, inset)) {
            const auto rect = D2D1::RectF(static_cast<float>(center.x)-inset, static_cast<float>(center.y)-inset,
                static_cast<float>(center.x)+inset, static_cast<float>(center.y)+inset);
            brush_->SetColor(ink); target_->FillRectangle(rect, brush_.Get());
            brush_->SetColor(D2D1::ColorF(0x2965B0)); target_->DrawRectangle(rect, brush_.Get(), dpi);
        }
        if (resizing) {
            const auto label = std::to_wstring(MulDiv(width_, 96, GetDpiForWindow(window))) + L" x "
                + std::to_wstring(MulDiv(height_, 96, GetDpiForWindow(window))) + L" DIP";
            const float cx = width_/2.0f, cy = height_/2.0f;
            const auto rect = D2D1::RectF(cx-65*dpi, cy-12*dpi, cx+65*dpi, cy+12*dpi);
            brush_->SetColor(D2D1::ColorF(0x101317, 0.95f)); target_->FillRectangle(rect, brush_.Get());
            // The format stays in DIP; scale the badge together with the monitor.
            target_->SetTransform(D2D1::Matrix3x2F::Scale(dpi, dpi));
            brush_->SetColor(ink);
            target_->DrawText(label.c_str(), static_cast<UINT32>(label.size()), format_.Get(),
                D2D1::RectF(cx/dpi-65, cy/dpi-12, cx/dpi+65, cy/dpi+12), brush_.Get());
        }
    }
    hr = target_->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) { release_surface(); return S_FALSE; }
    if (FAILED(hr)) return hr;
    SIZE size{static_cast<LONG>(width_), static_cast<LONG>(height_)};
    POINT origin{};
    const auto opacity=editing ? std::max(40,preferences_.overlay_opacity) : preferences_.overlay_opacity;
    BLENDFUNCTION blend{AC_SRC_OVER, 0, static_cast<BYTE>(opacity*255/100), AC_SRC_ALPHA};
    if (!UpdateLayeredWindow(window, nullptr, nullptr, &size, dc_, &origin, 0, &blend, ULW_ALPHA))
        return HRESULT_FROM_WIN32(GetLastError());
    return S_OK;
}
}
