// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <utility>
namespace overlay {
// Minimal owning COM pointer, keeping the renderer independent of Microsoft's
// optional WRL headers. Out parameters are used only on empty pointers.
template<class T> class ComPtr {
    T* pointer_{};
public:
    ComPtr() = default;
    ComPtr(const ComPtr&) = delete;
    ComPtr& operator=(const ComPtr&) = delete;
    ComPtr(ComPtr&& other) noexcept : pointer_(std::exchange(other.pointer_, nullptr)) {}
    ComPtr& operator=(ComPtr&& other) noexcept {
        if(this != &other) { Reset(); pointer_ = std::exchange(other.pointer_, nullptr); }
        return *this;
    }
    ~ComPtr() { Reset(); }
    void Reset() { if(pointer_) { pointer_->Release(); pointer_ = nullptr; } }
    T* Get() const { return pointer_; }
    T** GetAddressOf() { return &pointer_; }
    T* operator->() const { return pointer_; }
    explicit operator bool() const { return pointer_ != nullptr; }
};
}
