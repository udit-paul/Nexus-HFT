#pragma once

#include <cstddef>
#include <cassert>
#include <memory>
#include <type_traits>
#include <new>

namespace nexus {

/**
 * PoolAllocator<T>
 *
 * An STL-compatible stateful allocator backed by a fixed-size pre-allocated
 * memory arena. Designed to eliminate malloc/new on the hot path when used
 * with std::map for price level storage.
 *
 * Design:
 *   - Arena is a shared_ptr<ArenaState> so copies (required by STL rebind
 *     semantics) all point to the same underlying memory.
 *   - Free list is singly-linked, embedded in the free slots themselves.
 *   - O(1) allocate and deallocate — no system calls, no locks.
 *   - Falls back to ::operator new if the arena is exhausted (safety net).
 *
 * Thread-safety: NOT thread-safe. The matching engine is single-threaded.
 *
 * MSVC note: MSVC enforces that Allocator::value_type == container's value_type.
 * This is handled correctly via the rebind<U>::other mechanism.
 */

namespace detail {

// ── Shared arena state ─────────────────────────────────────────────────────
// All PoolAllocator<T> instances that were rebind-copied from the same root
// allocator share one ArenaState via a raw pointer (managed by the owner).
struct ArenaState {
    struct FreeNode { FreeNode* next; };

    char*       buffer   {nullptr};
    std::size_t slot_size{0};
    std::size_t capacity {0};
    FreeNode*   freelist {nullptr};

    void init(char* buf, std::size_t buf_bytes, std::size_t obj_size,
              std::size_t obj_align) noexcept {
        // Round up slot_size to alignment and ensure it can hold a FreeNode ptr
        slot_size = (obj_size + obj_align - 1) & ~(obj_align - 1);
        if (slot_size < sizeof(FreeNode)) slot_size = sizeof(FreeNode);
        capacity  = buf_bytes / slot_size;
        buffer    = buf;
        freelist  = nullptr;
        // Build freelist from high to low so first alloc returns slot 0
        for (std::size_t i = capacity; i-- > 0; ) {
            auto* node = reinterpret_cast<FreeNode*>(buffer + i * slot_size);
            node->next = freelist;
            freelist   = node;
        }
    }

    void* allocate() noexcept {
        if (!freelist) return nullptr;
        FreeNode* slot = freelist;
        freelist = slot->next;
        return static_cast<void*>(slot);
    }

    void deallocate(void* p) noexcept {
        auto* slot = static_cast<FreeNode*>(p);
        slot->next = freelist;
        freelist   = slot;
    }

    bool owns(const void* p) const noexcept {
        const char* cp = static_cast<const char*>(p);
        return cp >= buffer && cp < buffer + capacity * slot_size;
    }
};

} // namespace detail

// ── PoolAllocator<T> ──────────────────────────────────────────────────────────

template <typename T>
class PoolAllocator {
public:
    using value_type      = T;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;
    // Propagate so the map owns the allocator's state through moves/swaps
    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap            = std::true_type;

    template <typename U>
    struct rebind { using other = PoolAllocator<U>; };

    // ── Primary constructor ────────────────────────────────────────────────
    // Takes a pointer to a shared ArenaState (managed externally by OrderBook)
    explicit PoolAllocator(detail::ArenaState* arena) noexcept
        : arena_(arena) {}

    // ── Rebind constructor (called by std::map internally) ─────────────────
    template <typename U>
    PoolAllocator(const PoolAllocator<U>& other) noexcept
        : arena_(other.arena_) {}

    PoolAllocator(const PoolAllocator&) noexcept = default;
    PoolAllocator& operator=(const PoolAllocator&) noexcept = default;

    // ── STL allocator interface ────────────────────────────────────────────

    [[nodiscard]] T* allocate(std::size_t n) {
        // std::map only ever allocates 1 node at a time
        if (n == 1) {
            if (void* slot = arena_->allocate()) {
                return static_cast<T*>(slot);
            }
        }
        // Fallback: arena exhausted or bulk alloc (should not happen in normal use)
        return static_cast<T*>(::operator new(n * sizeof(T)));
    }

    void deallocate(T* ptr, std::size_t /*n*/) noexcept {
        if (arena_->owns(ptr)) {
            arena_->deallocate(ptr);
        } else {
            ::operator delete(ptr);
        }
    }

    bool operator==(const PoolAllocator& other) const noexcept {
        return arena_ == other.arena_;
    }
    bool operator!=(const PoolAllocator& other) const noexcept {
        return !(*this == other);
    }

    // Exposed so the rebind copy constructor of PoolAllocator<U> can access it
    detail::ArenaState* arena_{nullptr};
};

} // namespace nexus
