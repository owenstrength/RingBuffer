#include "buffer.hh"
#include "producer.hh"
#include "consumer.hh"
#include <benchmark/benchmark.h>

#include <thread>
#include <atomic>

template<auto ProducerFunc, auto ConsumerFunc>
static void RunBenchmark(benchmark::State& state, bool prefetch = false) {
    constexpr uint32_t N = 1 << 24;

    RingBuffer queue(prefetch);

    std::atomic<bool> done{false};
    std::atomic<uint64_t> work_epoch{0};
    std::atomic<uint64_t> producer_epoch{0};
    std::atomic<uint64_t> consumer_epoch{0};

    std::thread producer([&] {
        uint64_t local_epoch = 0;

        while (true) {
            while (work_epoch.load(std::memory_order_acquire) == local_epoch) {
                if (done.load(std::memory_order_acquire))
                    return;
            }

            local_epoch = work_epoch.load(std::memory_order_acquire);
            ProducerFunc(queue, N);
            producer_epoch.store(local_epoch, std::memory_order_release);
        }
    });

    std::thread consumer([&] {
        uint64_t local_epoch = 0;

        while (true) {
            while (work_epoch.load(std::memory_order_acquire) == local_epoch) {
                if (done.load(std::memory_order_acquire))
                    return;
            }

            local_epoch = work_epoch.load(std::memory_order_acquire);
            ConsumerFunc(queue, N);
            consumer_epoch.store(local_epoch, std::memory_order_release);
        }
    });

    for (auto _ : state) {
        const uint64_t next_epoch = work_epoch.load(std::memory_order_relaxed) + 1;
        work_epoch.store(next_epoch, std::memory_order_release);

        while (producer_epoch.load(std::memory_order_acquire) != next_epoch) {}
        while (consumer_epoch.load(std::memory_order_acquire) != next_epoch) {}
    }

    done.store(true, std::memory_order_release);
    work_epoch.fetch_add(1, std::memory_order_release);

    producer.join();
    consumer.join();

    state.SetItemsProcessed(state.iterations() * N);
}

static void BM_SPSCQueue(benchmark::State& state) {
    RunBenchmark<busy_producer, busy_consumer>(state);
}

static void BM_SPSCQueueCache(benchmark::State& state) {
    RunBenchmark<busy_producer_cache, busy_consumer_cache>(state);
}

static void BM_SPSCQueuePrefetch(benchmark::State& state) {
    RunBenchmark<busy_producer, busy_consumer_prefetch>(state, true);
}

static void BM_SPSCQueueCachePrefetch(benchmark::State& state) {
    RunBenchmark<busy_producer_cache, busy_consumer_cache_prefetch>(state, true);
}

BENCHMARK(BM_SPSCQueue);
BENCHMARK(BM_SPSCQueuePrefetch);
BENCHMARK(BM_SPSCQueueCache);
BENCHMARK(BM_SPSCQueueCachePrefetch);
BENCHMARK_MAIN();
