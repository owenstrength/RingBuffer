#pragma once

inline void cpu_relax() {
#if defined(__x86_64__) || defined(__i386__)
    _mm_pause();
#elif defined(__aarch64__) || defined(__arm__)
    asm volatile("yield");
#else
    std::this_thread::yield();
#endif
}