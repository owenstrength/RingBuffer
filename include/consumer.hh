#pragma once

#include "buffer.hh"
#include "common.hh"
#include <iostream>

static inline void busy_consumer(RingBuffer& queue, uint32_t items) {
    uint32_t value;
    for (uint32_t i = 0; i < items; ++i) {
        while (!queue.pop(value)) {
            cpu_relax(); // wait for producer to push items, reduce CPU contention
        }
        #ifdef DEBUG
        std::cout << "Consumed: " << value << std::endl;
        #endif
    }
}

static inline void busy_consumer_prefetch(RingBuffer& queue, uint32_t items) {
    uint32_t value;
    for (uint32_t i = 0; i < items; ++i) {
        while (!queue.pop(value)) {
            cpu_relax(); // wait for producer to push items, reduce CPU contention
        }
        #ifdef DEBUG
        std::cout << "Consumed: " << value << std::endl;
        #endif
    }
}

static inline void busy_consumer_cache(RingBuffer& queue, uint32_t items) {
    uint32_t tail = queue.tail.load(std::memory_order_relaxed);
    uint32_t head_cache = queue.head.load(std::memory_order_relaxed);

    for (uint32_t i = 0; i < items; i++) {

        if (tail == head_cache) {
            head_cache = queue.head.load(std::memory_order_acquire);
            while (tail == head_cache)
                head_cache = queue.head.load(std::memory_order_acquire);
        }

        uint32_t value = queue.buffer[tail & queue.BUFFER_MASK];
        #ifdef DEBUG
        std::cout << "Consumed: " << value << std::endl;
        #else
        (void)value;
        #endif

        tail++;
        queue.tail.store(tail, std::memory_order_release);
    }
}

static inline void busy_consumer_cache_prefetch(RingBuffer& queue, uint32_t items) {
    uint32_t tail = queue.tail.load(std::memory_order_relaxed);
    uint32_t head_cache = queue.head.load(std::memory_order_relaxed);

    for (uint32_t i = 0; i < items; i++) {

        if (tail == head_cache) {
            head_cache = queue.head.load(std::memory_order_acquire);
            while (tail == head_cache)
                head_cache = queue.head.load(std::memory_order_acquire);
        }

        uint32_t value = queue.buffer[tail & queue.BUFFER_MASK];
        if (queue.prefetch_enabled)
            __builtin_prefetch(&queue.buffer[(tail + 64) & queue.BUFFER_MASK]);
        #ifdef DEBUG
        std::cout << "Consumed: " << value << std::endl;
        #else
        (void)value;
        #endif

        tail++;
        queue.tail.store(tail, std::memory_order_release);
    }
}
