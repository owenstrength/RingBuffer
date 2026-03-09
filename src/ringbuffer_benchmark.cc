#include "buffer.hh"
#include "buffer_cache.hh"
#include "producer.hh"
#include "consumer.hh"
#include <benchmark/benchmark.h>

#include <thread>
#include <atomic>

template <typename Queue>
static void RunBenchmark(benchmark::State& state) {
    constexpr uint32_t N = 1 << 20;

    Queue queue;

    std::atomic<bool> start{false};
    std::atomic<bool> done{false};

    std::thread producer([&] {
        while (!start.load(std::memory_order_acquire)) {}

        while (!done.load(std::memory_order_acquire)) {
            busy_producer<Queue>(queue, N);
        }
    });

    std::thread consumer([&] {
        while (!start.load(std::memory_order_acquire)) {}

        while (!done.load(std::memory_order_acquire)) {
            busy_consumer<Queue>(queue, N);
        }
    });

    start.store(true, std::memory_order_release);

    for (auto _ : state) {
        busy_producer<Queue>(queue, N);
        busy_consumer<Queue>(queue, N);
    }

    done.store(true, std::memory_order_release);

    producer.join();
    consumer.join();

    state.SetItemsProcessed(state.iterations() * N);
}

static void BM_SPSCQueue(benchmark::State& state) {
    RunBenchmark<RingBuffer>(state);
}

static void BM_SPSCQueueCache(benchmark::State& state) {
    RunBenchmark<RingBufferCache>(state);
}

BENCHMARK(BM_SPSCQueue);
BENCHMARK(BM_SPSCQueueCache);
BENCHMARK_MAIN();