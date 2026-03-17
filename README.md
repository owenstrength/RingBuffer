# Single Producer Single Consumer Queue (Ring Buffer)

A simple single-producer/single-consumer (SPSC) lock-free ring buffer in C++ with a Google Benchmark harness.

## Features

- Lock-free SPSC queue using atomics
- Cache-line alignment for hot fields (`head`, `tail`)
- Optional consumer-side prefetch path
- Benchmark target with multiple queue variants

## Project Layout

- `include/buffer.hh`: ring buffer implementation
- `include/producer.hh`: producer workloads
- `include/consumer.hh`: consumer workloads
- `src/main.cc`: simple threaded demo
- `src/ringbuffer_benchmark.cc`: Google Benchmark suite

## Build

```bash
cmake -S . -B build
cmake --build build
```

Binaries are written to `bin/`.

## Run

Run the demo:

```bash
./bin/ringbuffer
```

Run benchmarks:

```bash
./bin/ringbuffer_benchmark
```

Filter benchmarks:

```bash
./bin/ringbuffer_benchmark --benchmark_filter=Prefetch
```

## Benchmark Results

**Aggregated over 10 runs** on a 6-core ARM system (48 MHz each, L2 4096 KiB shared):

| Benchmark | Mean Time (ns) | Median Time (ns) | Stddev (ns) | Throughput |
|-----------|----------------|-----------------|-------------|------------|
| `BM_SPSCQueue` | 151,771,953 | 154,002,522 | ±10,367,329 (6.8%) | 120.46 M/s |
| `BM_SPSCQueuePrefetch` | 161,255,364 | 163,877,626 | ±11,947,562 (7.4%) | 113.42 M/s |
| `BM_SPSCQueueCache` | 195,162,824 | 170,450,866 | ±57,642,003 (29.5%) | 101.55 M/s |
| `BM_SPSCQueueCachePrefetch` | 147,266,252 | 154,464,112 | ±28,919,517 (19.6%) | 130.13 M/s |
| `BM_SPSCQueueCacheBatch` | **21,459,051** | 21,306,196 | ±2,815,161 (13.1%) | **861.12 M/s** ⚡ |
| `BM_SPSCQueueCachePrefetchBatch` | 21,987,863 | 23,002,471 | ±2,518,218 (11.5%) | 839.51 M/s ⚡ |

### Key Observations

- **Baseline**: 120 M/s on simple atomic push/pop.
- **Cache-optimized with prefetch** (`CachePrefetch`): **+8% throughput** over baseline—best non-batch variant.
- **Standalone prefetch**: Negligible *(-6%)* — hidden under scheduler noise.
- **Cache variance**: High stddev (*29%*) indicates epoch-scheduler interactions under load.
- **Batch mode (⚡)**: **7x throughput boost** (861 M/s) by amortizing atomic updates—dramatic win for high-contention scenarios.

## Notes

- CMake tries to use a system-installed Google Benchmark first.
- If not found, it fetches Google Benchmark automatically via `FetchContent`.
- Run benchmarks with `--benchmark_repetitions=10 --benchmark_report_aggregates_only=true` for aggregated stats.
