// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>
namespace overlay::placement {
struct Point { double x, y; };
struct Rect {
    double x, y, w, h;
    double right() const { return x + w; }
    double bottom() const { return y + h; }
};
enum class Handle { None, Move, NW, N, NE, E, SE, S, SW, W };
struct Limits { double min_w, min_h, max_w, max_h; };
inline constexpr double aspect = 338.0 / 148.0;
inline std::array<Point, 8> handles(double w, double h, double inset) {
    return {{{inset, inset}, {w/2, inset}, {w-inset, inset}, {w-inset, h/2},
        {w-inset, h-inset}, {w/2, h-inset}, {inset, h-inset}, {inset, h/2}}};
}
inline Handle hit(Point p, double w, double h, double dpi_scale) {
    const auto centers = handles(w, h, 4 * dpi_scale);
    constexpr std::array<Handle, 8> ids{Handle::NW, Handle::N, Handle::NE, Handle::E,
        Handle::SE, Handle::S, Handle::SW, Handle::W};
    for (unsigned i = 0; i < centers.size(); ++i)
        if (std::abs(p.x-centers[i].x) <= 5*dpi_scale && std::abs(p.y-centers[i].y) <= 5*dpi_scale) return ids[i];
    return p.x >= 0 && p.y >= 0 && p.x < w && p.y < h ? Handle::Move : Handle::None;
}
inline Rect drag(Rect start, Handle handle, double dx, double dy, bool aspect_lock,
    bool shift, Limits limits) {
    if (handle == Handle::Move) return {start.x+dx, start.y+dy, start.w, start.h};
    const bool left = handle == Handle::W || handle == Handle::NW || handle == Handle::SW;
    const bool right = handle == Handle::E || handle == Handle::NE || handle == Handle::SE;
    const bool top = handle == Handle::N || handle == Handle::NW || handle == Handle::NE;
    const bool bottom = handle == Handle::S || handle == Handle::SW || handle == Handle::SE;
    if (!(left || right || top || bottom)) return start;
    const bool corner = (left || right) && (top || bottom);
    double w = start.w + (left ? -dx : right ? dx : 0);
    double h = start.h + (top ? -dy : bottom ? dy : 0);
    if (aspect_lock || shift || corner) {
        const double ratio = (aspect_lock || shift) ? aspect : start.w/start.h;
        if ((top || bottom) && (!(left || right) || std::abs(h/start.h-1) > std::abs(w/start.w-1))) w = h * ratio;
        const double low = std::max(limits.min_w, limits.min_h*ratio);
        const double high = std::min(limits.max_w, limits.max_h*ratio);
        w = std::clamp(w, low, high); h = w/ratio;
    } else {
        w = std::clamp(w, limits.min_w, limits.max_w);
        h = std::clamp(h, limits.min_h, limits.max_h);
    }
    // Opposite corner/edge stays anchored; locked edge resize grows symmetrically
    // on the orthogonal axis. Flipping through the anchor is prevented by minima.
    return {left ? start.right()-w : right ? start.x : start.x+(start.w-w)/2,
        top ? start.bottom()-h : bottom ? start.y : start.y+(start.h-h)/2, w, h};
}
inline bool accessible(Rect rect, const std::vector<Rect>& screens) {
    for (auto screen : screens)
        if (std::min(rect.right(), screen.right())-std::max(rect.x, screen.x) >= std::min(48.0, rect.w)
            && std::min(rect.bottom(), screen.bottom())-std::max(rect.y, screen.y) >= std::min(32.0, rect.h)) return true;
    return false;
}
inline Rect recover(Rect rect, const std::vector<Rect>& screens, bool force = false) {
    if (screens.empty() || (!force && accessible(rect, screens))) return rect;
    auto nearest = screens.front();
    auto distance = [&](Rect screen) {
        const auto x = std::clamp(rect.x+rect.w/2, screen.x, screen.right());
        const auto y = std::clamp(rect.y+rect.h/2, screen.y, screen.bottom());
        return std::hypot(x-rect.x-rect.w/2, y-rect.y-rect.h/2);
    };
    for (auto screen : screens) if (distance(screen) < distance(nearest)) nearest = screen;
    const double scale = std::min({1.0, nearest.w/rect.w, nearest.h/rect.h});
    rect.w *= scale; rect.h *= scale;
    rect.x = std::clamp(rect.x, nearest.x, nearest.right()-rect.w);
    rect.y = std::clamp(rect.y, nearest.y, nearest.bottom()-rect.h);
    return rect;
}
}
