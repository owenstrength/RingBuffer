#pragma once

#include "buffer.hh"
#include "common.hh"
#include <iostream>

template <typename BufferType>
static inline void busy_consumer(BufferType& queue, uint32_t items) {
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