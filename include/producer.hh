#pragma once

#include "buffer.hh"
#include "common.hh"

template <typename BufferType>
static inline void busy_producer(BufferType& queue, uint32_t items) {
    for (uint32_t i = 0; i < items; ++i) {
        while (!queue.push(i)) {
            cpu_relax(); // wait for consumer to pop items, reduce CPU contention
        }
    }
}
