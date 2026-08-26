#pragma once

#include <atomic>
#include <cassert>
#include <cstddef>
#include <vector>

namespace nexus {

// Hardware destructive interference size (typically 64 bytes for x86)
constexpr std::size_t CacheLineSize = 64;

/**
 * A lock-free, wait-free Single-Producer Single-Consumer (SPSC) Ring Buffer.
 * Cache-aligned to prevent false sharing between producer and consumer threads.
 */
template <typename T>
class alignas(CacheLineSize) SPSCQueue {
public:
    // Capacity must be a power of 2 for fast modulo arithmetic (bitwise AND)
    explicit SPSCQueue(std::size_t capacity) 
        : capacity_(capacity), 
          mask_(capacity - 1), 
          buffer_(capacity) {
        
        // Assert capacity is a power of 2
        assert(capacity > 0 && (capacity & (capacity - 1)) == 0);
    }

    // Called by Producer thread
    template <typename U>
    bool push(U&& item) {
        const std::size_t head = head_.load(std::memory_order_relaxed);
        const std::size_t next_head = (head + 1) & mask_;

        // Check if queue is full (using cached tail to minimize atomic loads)
        if (next_head == cached_tail_) {
            cached_tail_ = tail_.load(std::memory_order_acquire);
            if (next_head == cached_tail_) {
                return false; // Queue is full
            }
        }

        buffer_[head] = std::forward<U>(item);
        
        // Release semantics ensure the write to buffer is visible before head is updated
        head_.store(next_head, std::memory_order_release);
        return true;
    }

    // Called by Consumer thread
    bool pop(T& item) {
        const std::size_t tail = tail_.load(std::memory_order_relaxed);

        // Check if queue is empty (using cached head)
        if (tail == cached_head_) {
            cached_head_ = head_.load(std::memory_order_acquire);
            if (tail == cached_head_) {
                return false; // Queue is empty
            }
        }

        item = std::move(buffer_[tail]);
        
        // Release semantics ensure the read from buffer is finished before tail is updated
        tail_.store((tail + 1) & mask_, std::memory_order_release);
        return true;
    }

private:
    const std::size_t capacity_;
    const std::size_t mask_;
    std::vector<T> buffer_;

    // Cache-aligned Producer state
    alignas(CacheLineSize) std::atomic<std::size_t> head_{0};
    alignas(CacheLineSize) std::size_t cached_tail_{0};

    // Cache-aligned Consumer state
    alignas(CacheLineSize) std::atomic<std::size_t> tail_{0};
    alignas(CacheLineSize) std::size_t cached_head_{0};
};

} // namespace nexus
