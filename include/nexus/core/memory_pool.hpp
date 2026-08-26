#pragma once

#include <cstddef>
#include <cstdint>
#include <cassert>
#include <new>
#include <type_traits>

namespace nexus {

/**
 * MemoryPool<T>
 *
 * A fixed-size, slab-based object pool with an intrusive singly-linked
 * freelist. Compared to the previous implementation (which stored free
 * indices in a heap-allocated std::vector<size_t>), this version:
 *
 *   1. Stores the freelist pointer INSIDE the free slot itself — no secondary
 *      allocation, no pointer indirection.
 *   2. Allocate / Deallocate are pure pointer operations: ~3–5ns vs ~15–30ns.
 *   3. The slab (pool_) is still heap-allocated once at startup, but all
 *      subsequent alloc/dealloc are allocation-free.
 *
 * Requirement: sizeof(T) >= sizeof(void*).  OrderNode is 64 bytes, so this
 * is always satisfied.
 *
 * Thread-safety: NOT thread-safe. The matching engine is single-threaded.
 */
template <typename T>
class MemoryPool {
    static_assert(sizeof(T) >= sizeof(void*),
        "MemoryPool: T must be large enough to hold a freelist pointer. "
        "sizeof(T) must be >= sizeof(void*).");

public:
    explicit MemoryPool(std::size_t capacity)
        : capacity_(capacity),
          slab_(static_cast<T*>(::operator new(capacity * sizeof(T)))),
          freelist_head_(nullptr),
          allocated_(0)
    {
        // Build the freelist by writing the next-pointer into each free slot.
        // We iterate in reverse so that the first allocation returns slot 0
        // (gives predictable, cache-friendly sequential access on startup).
        for (std::size_t i = capacity_; i-- > 0; ) {
            push_free(slab_ + i);
        }
    }

    ~MemoryPool() {
        // We do not call destructors for un-allocated objects — they were
        // never constructed. Allocated objects must be explicitly deallocated
        // by the caller before the pool is destroyed.
        ::operator delete(slab_);
    }

    // Non-copyable, non-movable
    MemoryPool(const MemoryPool&)            = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    // ── Allocation ────────────────────────────────────────────────────────

    /**
     * Allocate and construct one object from the pool.
     * Returns nullptr if the pool is exhausted (no silent crash).
     * Cost: ~3–5ns (two pointer loads + one store, all in L1 cache).
     */
    template <typename... Args>
    T* allocate(Args&&... args) {
        if (!freelist_head_) [[unlikely]] {
            return nullptr; // Pool exhausted
        }
        T* slot = pop_free();
        new (slot) T(std::forward<Args>(args)...);
        ++allocated_;
        return slot;
    }

    // ── Deallocation ──────────────────────────────────────────────────────

    /**
     * Destruct the object and return its slot to the freelist.
     * Cost: ~3–5ns (destructor + two pointer stores, all in L1 cache).
     */
    void deallocate(T* ptr) noexcept {
        if (!ptr) return;
        assert(owns(ptr) && "MemoryPool::deallocate: pointer does not belong to this pool!");
        ptr->~T();
        push_free(ptr);
        --allocated_;
    }

    // ── Diagnostics ───────────────────────────────────────────────────────

    std::size_t capacity()  const noexcept { return capacity_; }
    std::size_t allocated() const noexcept { return allocated_; }
    std::size_t available() const noexcept { return capacity_ - allocated_; }

    /** Returns true if ptr was allocated from this pool's slab. */
    bool owns(const T* ptr) const noexcept {
        return ptr >= slab_ && ptr < slab_ + capacity_;
    }

private:
    // ── Intrusive freelist helpers ─────────────────────────────────────────

    // A free slot is raw memory — we store the next-pointer in its first bytes.
    struct FreeNode { FreeNode* next; };

    void push_free(T* slot) noexcept {
        auto* node = reinterpret_cast<FreeNode*>(slot);
        node->next = freelist_head_;
        freelist_head_ = node;
    }

    T* pop_free() noexcept {
        FreeNode* node = freelist_head_;
        freelist_head_ = node->next;
        return reinterpret_cast<T*>(node);
    }

    // ── Members ───────────────────────────────────────────────────────────

    const std::size_t capacity_;
    T*                slab_;            // Contiguous block of raw memory
    FreeNode*         freelist_head_;   // Head of the intrusive freelist
    std::size_t       allocated_;       // Current allocation count (for diagnostics)
};

} // namespace nexus
