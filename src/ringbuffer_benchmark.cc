#include "buffer.hh"
#include "producer.hh"
#include "consumer.hh"
#include <benchmark/benchmark.h>

#include <thread>
#include <atomic>

#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#endif

static inline void spin_pause() {
#if defined(__x86_64__) || defined(__i386__)
    _mm_pause();
#else
    std::this_thread::yield();
#endif
}

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
            if (done.load(std::memory_order_acquire))
                return;

            const uint64_t current_epoch = work_epoch.load(std::memory_order_acquire);
            if (current_epoch == local_epoch) {
                spin_pause();
                continue;
            }

            local_epoch = current_epoch;
            if (done.load(std::memory_order_acquire))
                return;

            ProducerFunc(queue, N);
            producer_epoch.store(local_epoch, std::memory_order_release);
        }
    });

    std::thread consumer([&] {
        uint64_t local_epoch = 0;

        while (true) {
            if (done.load(std::memory_order_acquire))
                return;

            const uint64_t current_epoch = work_epoch.load(std::memory_order_acquire);
            if (current_epoch == local_epoch) {
                spin_pause();
                continue;
            }

            local_epoch = current_epoch;
            if (done.load(std::memory_order_acquire))
                return;

            ConsumerFunc(queue, N);
            consumer_epoch.store(local_epoch, std::memory_order_release);
        }
    });

    for (auto _ : state) {
        const uint64_t next_epoch = work_epoch.load(std::memory_order_relaxed) + 1;
        work_epoch.store(next_epoch, std::memory_order_release);

        while (producer_epoch.load(std::memory_order_acquire) != next_epoch)
            spin_pause();
        while (consumer_epoch.load(std::memory_order_acquire) != next_epoch)
            spin_pause();
    }

    done.store(true, std::memory_order_seq_cst);
    work_epoch.fetch_add(1, std::memory_order_seq_cst);

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
    RunBenchmark<busy_producer, busy_consumer>(state, true);
}

static void BM_SPSCQueueCachePrefetch(benchmark::State& state) {
    RunBenchmark<busy_producer_cache, busy_consumer_cache>(state, true);
}

static void BM_SPSCQueueCacheBatch(benchmark::State& state) {
    RunBenchmark<busy_producer_cache_batch, busy_consumer_cache_batch>(state);
}

static void BM_SPSCQueueCachePrefetchBatch(benchmark::State& state) {
    RunBenchmark<busy_producer_cache_batch, busy_consumer_cache_batch>(state, true);
}

BENCHMARK(BM_SPSCQueue);
BENCHMARK(BM_SPSCQueuePrefetch);
BENCHMARK(BM_SPSCQueueCache);
BENCHMARK(BM_SPSCQueueCachePrefetch);
BENCHMARK(BM_SPSCQueueCacheBatch);
BENCHMARK(BM_SPSCQueueCachePrefetchBatch);
BENCHMARK_MAIN();
