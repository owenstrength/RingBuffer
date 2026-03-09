#pragma once

#include "buffer.hh"
#include "common.hh"

static inline void busy_producer(RingBuffer& queue, uint32_t items) {
    for (uint32_t i = 0; i < items; ++i) {
        while (!queue.push(i)) {
            cpu_relax(); // wait for consumer to pop items, reduce CPU contention
        }
    }
}

static inline void busy_producer_cache(RingBuffer& queue, uint32_t items) {
    uint32_t head = queue.head.load(std::memory_order_relaxed);
    uint32_t tail_cache = queue.tail.load(std::memory_order_acquire);

    for (uint32_t i = 0; i < items; ++i) {
        if (head - tail_cache == queue.BUFFER_SIZE) {
            do {
                cpu_relax();
                tail_cache = queue.tail.load(std::memory_order_acquire);
            } while (head - tail_cache == queue.BUFFER_SIZE);
        }

        queue.buffer[head & queue.BUFFER_MASK] = i;
        ++head;
        queue.head.store(head, std::memory_order_release);
    }
}


static inline void busy_producer_cache_batch(RingBuffer& queue, uint32_t items) {
    uint32_t head = queue.head.load(std::memory_order_relaxed);
    uint32_t tail_cache = queue.tail.load(std::memory_order_acquire);

    constexpr uint32_t BATCH = 32;
    uint32_t published_head = head;

    for (uint32_t i = 0; i < items; ++i) {
        if (head - tail_cache == queue.BUFFER_SIZE) {
            queue.head.store(head, std::memory_order_release);
            published_head = head;

            do {
                cpu_relax();
                tail_cache = queue.tail.load(std::memory_order_acquire);
            } while (head - tail_cache == queue.BUFFER_SIZE);
        }

        queue.buffer[head & queue.BUFFER_MASK] = i;
        ++head;

        if (head - published_head >= BATCH) {
            queue.head.store(head, std::memory_order_release);
            published_head = head;
        }
    }

    if (published_head != head) {
        queue.head.store(head, std::memory_order_release);
    }
}