#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

struct RingBuffer {
    const bool prefetch_enabled;
    static constexpr size_t CACHE_LINE_SIZE = 64;
    static constexpr uint32_t BUFFER_SIZE = 4096;

    explicit RingBuffer(bool prefetch = false) : prefetch_enabled(prefetch) {}

    // ensure buffer size is a power of 2
    static_assert(BUFFER_SIZE > 1 && (BUFFER_SIZE & (BUFFER_SIZE - 1)) == 0);

    static constexpr uint32_t BUFFER_MASK = BUFFER_SIZE - 1;

    alignas(CACHE_LINE_SIZE) 
    std::atomic<uint32_t> head{0};

    alignas(CACHE_LINE_SIZE)
    std::atomic<uint32_t> tail{0};

    alignas(CACHE_LINE_SIZE)
    uint32_t buffer[BUFFER_SIZE]{};

    bool empty() const {
        return head.load(std::memory_order_acquire) == tail.load(std::memory_order_acquire);
    }

    bool full() const {
        return head.load(std::memory_order_acquire) -
               tail.load(std::memory_order_acquire) == BUFFER_SIZE;
    }

    bool push(uint32_t value) {
        uint32_t h = head.load(std::memory_order_acquire);

        if (h - tail.load(std::memory_order_acquire) == BUFFER_SIZE)
            return false;

        buffer[h & BUFFER_MASK] = value;
        head.store(h + 1, std::memory_order_release);
        return true;
    }

    bool pop(uint32_t& value) {
        uint32_t t = tail.load(std::memory_order_acquire);

        if (t == head.load(std::memory_order_acquire))
            return false;

        value = buffer[t & BUFFER_MASK];
        if (prefetch_enabled)
            __builtin_prefetch(&buffer[(t + 256) & BUFFER_MASK]);

        t += 1;
        tail.store(t, std::memory_order_release);
        return true;
    }

};
