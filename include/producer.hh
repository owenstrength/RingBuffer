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
    uint32_t tail_cache = queue.tail.load(std::memory_order_relaxed);

    for (uint32_t i = 0; i < items; i++) {

        if (head - tail_cache == queue.BUFFER_SIZE) {
            tail_cache = queue.tail.load(std::memory_order_acquire);
            while (head - tail_cache == queue.BUFFER_SIZE)
                tail_cache = queue.tail.load(std::memory_order_acquire);
        }

        queue.buffer[head & queue.BUFFER_MASK] = i;

        head++;
        queue.head.store(head, std::memory_order_release);
    }
}
