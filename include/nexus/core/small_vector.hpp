#pragma once

#include <cstddef>
#include <cstring>
#include <cassert>
#include <utility>
#include <type_traits>
#include <iterator>
#include <new>

namespace nexus {

/**
 * SmallVector<T, N>
 *
 * A drop-in replacement for std::vector<T> that stores up to N elements
 * inline (on the stack / inside the parent struct) without any heap allocation.
 * Only if the element count exceeds N does it fall back to heap — matching the
 * exact behaviour of LLVM's SmallVector.
 *
 * Key HFT properties:
 *   - Zero malloc() calls for ≤N elements (eliminates the single biggest
 *     latency source in process_limit_order).
 *   - Trivially destructible for POD types (T=Trade).
 *   - Supports emplace_back, size, operator[], range-for, data/begin/end.
 */
template <typename T, std::size_t N>
class SmallVector {
    static_assert(N > 0, "SmallVector inline capacity must be > 0");

public:
    using value_type      = T;
    using size_type       = std::size_t;
    using reference       = T&;
    using const_reference = const T&;
    using pointer         = T*;
    using const_pointer   = const T*;
    using iterator        = T*;
    using const_iterator  = const T*;

    // ── Construction / Destruction ─────────────────────────────────────────

    SmallVector() noexcept : size_(0), capacity_(N), heap_data_(nullptr) {}

    ~SmallVector() {
        destroy_elements();
        if (is_heap()) {
            ::operator delete(heap_data_);
        }
    }

    // Non-copyable for now (HFT code should move, not copy vectors of Trades)
    SmallVector(const SmallVector&) = delete;
    SmallVector& operator=(const SmallVector&) = delete;

    SmallVector(SmallVector&& other) noexcept
        : size_(other.size_), capacity_(other.capacity_), heap_data_(nullptr) {
        if (other.is_heap()) {
            heap_data_  = other.heap_data_;
            other.heap_data_ = nullptr;
        } else {
            // Move inline elements into our inline storage
            T* src = other.inline_ptr();
            T* dst = inline_ptr();
            for (std::size_t i = 0; i < size_; ++i) {
                new (dst + i) T(std::move(src[i]));
                src[i].~T();
            }
        }
        other.size_ = 0;
        other.capacity_ = N;
    }

    // ── Element access ─────────────────────────────────────────────────────

    T*       data()       noexcept { return is_heap() ? heap_data_ : inline_ptr(); }
    const T* data() const noexcept { return is_heap() ? heap_data_ : inline_ptr(); }

    T& operator[](std::size_t i)       noexcept { return data()[i]; }
    const T& operator[](std::size_t i) const noexcept { return data()[i]; }

    T& back()       noexcept { return data()[size_ - 1]; }
    const T& back() const noexcept { return data()[size_ - 1]; }

    // ── Iterators ──────────────────────────────────────────────────────────

    iterator       begin()       noexcept { return data(); }
    iterator       end()         noexcept { return data() + size_; }
    const_iterator begin() const noexcept { return data(); }
    const_iterator end()   const noexcept { return data() + size_; }
    const_iterator cbegin() const noexcept { return begin(); }
    const_iterator cend()   const noexcept { return end(); }

    // ── Capacity ───────────────────────────────────────────────────────────

    std::size_t size()     const noexcept { return size_; }
    std::size_t capacity() const noexcept { return capacity_; }
    bool        empty()    const noexcept { return size_ == 0; }

    // ── Modifiers ──────────────────────────────────────────────────────────

    template <typename... Args>
    T& emplace_back(Args&&... args) {
        if (size_ == capacity_) {
            grow();
        }
        T* slot = data() + size_;
        new (slot) T(std::forward<Args>(args)...);
        ++size_;
        return *slot;
    }

    void push_back(const T& v) { emplace_back(v); }
    void push_back(T&& v)      { emplace_back(std::move(v)); }

    void clear() noexcept {
        destroy_elements();
        size_ = 0;
    }

private:
    // ── Internal layout ────────────────────────────────────────────────────

    std::size_t size_;
    std::size_t capacity_;
    T*          heap_data_; // nullptr when using inline storage

    // Inline storage: properly aligned raw bytes for N objects of type T
    alignas(T) std::byte inline_storage_[sizeof(T) * N];

    T* inline_ptr() noexcept {
        return reinterpret_cast<T*>(inline_storage_);
    }
    const T* inline_ptr() const noexcept {
        return reinterpret_cast<const T*>(inline_storage_);
    }

    bool is_heap() const noexcept { return heap_data_ != nullptr; }

    void destroy_elements() noexcept {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            T* p = data();
            for (std::size_t i = 0; i < size_; ++i) {
                p[i].~T();
            }
        }
        // For trivially destructible T (e.g. Trade), this is a no-op.
    }

    // Grow strategy: double capacity, move elements to heap.
    void grow() {
        std::size_t new_cap = capacity_ * 2;
        T* new_buf = static_cast<T*>(::operator new(new_cap * sizeof(T)));

        T* old = data();
        for (std::size_t i = 0; i < size_; ++i) {
            new (new_buf + i) T(std::move(old[i]));
            old[i].~T();
        }

        if (is_heap()) {
            ::operator delete(heap_data_);
        }

        heap_data_ = new_buf;
        capacity_  = new_cap;
    }
};

} // namespace nexus
