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

static inline void busy_consumer_cache(RingBuffer& queue, uint32_t items) {
    uint32_t tail = queue.tail.load(std::memory_order_relaxed);
    uint32_t head_cache = queue.head.load(std::memory_order_acquire);

    for (uint32_t i = 0; i < items; ++i) {
        if (tail == head_cache) {
            queue.tail.store(tail, std::memory_order_release);

            do {
                cpu_relax();
                head_cache = queue.head.load(std::memory_order_acquire);
            } while (tail == head_cache);
        }

        uint32_t value = queue.buffer[tail & queue.BUFFER_MASK];
        (void)value;
        ++tail;

        queue.tail.store(tail, std::memory_order_release);
    }
}

static inline void busy_consumer_cache_batch(RingBuffer& queue, uint32_t items) {
    uint32_t tail = queue.tail.load(std::memory_order_relaxed);
    uint32_t head_cache = queue.head.load(std::memory_order_acquire);

    constexpr uint32_t BATCH = 32;
    uint32_t published_tail = tail;

    for (uint32_t i = 0; i < items; ++i) {
        if (tail == head_cache) {
            queue.tail.store(tail, std::memory_order_release);
            published_tail = tail;

            do {
                cpu_relax();
                head_cache = queue.head.load(std::memory_order_acquire);
            } while (tail == head_cache);
        }

        uint32_t value = queue.buffer[tail & queue.BUFFER_MASK];
        (void)value;
        ++tail;

        if (tail - published_tail >= BATCH) {
            queue.tail.store(tail, std::memory_order_release);
            published_tail = tail;
        }
    }

    if (published_tail != tail) {
        queue.tail.store(tail, std::memory_order_release);
    }
}