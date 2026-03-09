#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <new>
#include <memory>


struct RingBufferCache {
    static constexpr size_t CACHE_LINE_SIZE = 64;
    static constexpr uint32_t BUFFER_SIZE = 4096;

    // ensure buffer size is a power of 2
    static_assert(BUFFER_SIZE > 1 && (BUFFER_SIZE & (BUFFER_SIZE - 1)) == 0);

    static constexpr uint32_t BUFFER_MASK = BUFFER_SIZE - 1;

    alignas(CACHE_LINE_SIZE) 
    std::atomic<uint32_t> head{0};

    alignas(CACHE_LINE_SIZE)
    std::atomic<uint32_t> tail{0};

    alignas(CACHE_LINE_SIZE)
    uint32_t buffer[BUFFER_SIZE]{};

    alignas(CACHE_LINE_SIZE)
    uint32_t head_cache{0};

    alignas(CACHE_LINE_SIZE)
    uint32_t tail_cache{0};

    bool empty() const {
        return head.load(std::memory_order_acquire) == tail.load(std::memory_order_acquire);
    }

    bool full() const {
        uint32_t next = (head.load(std::memory_order_acquire) + 1) % BUFFER_SIZE;
        return next == tail.load(std::memory_order_acquire);
    }

    bool push(uint32_t value) {
        uint32_t h = head_cache;

        if (h - tail_cache == BUFFER_SIZE &&
            (tail_cache = tail.load(std::memory_order_acquire), h - tail_cache == BUFFER_SIZE))
            return false;

        buffer[h & BUFFER_MASK] = value;
        head_cache = h + 1;
        head.store(head_cache, std::memory_order_release);
        return true;
    }

    bool pop(uint32_t& value) {
        uint32_t t = tail_cache;

        if (t == head_cache && (head_cache = head.load(std::memory_order_acquire)) == t)
            return false;

        value = buffer[t & BUFFER_MASK];
        __builtin_prefetch(&buffer[(t + 4) & BUFFER_MASK]); // prefetch next items

        t += 1;
        tail_cache = t;
        tail.store(t, std::memory_order_release);
        return true;
    }

};