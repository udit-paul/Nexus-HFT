#include <benchmark/benchmark.h>
#include "nexus/concurrency/spsc_queue.hpp"
#include "nexus/concurrency/cpu_affinity.hpp"
#include <thread>
#include <atomic>

using namespace nexus;

// Benchmark for Single Producer Single Consumer Queue
static void BM_SPSCQueue_Throughput(benchmark::State& state) {
    constexpr std::size_t QUEUE_SIZE = 1024 * 64; // 64k entries
    SPSCQueue<uint64_t> queue(QUEUE_SIZE);

    std::atomic<bool> producer_running{true};
    
    // Spawn producer thread
    std::thread producer([&]() {
        // Pin to Core 0 (or first available)
        pin_thread_to_core(0);
        uint64_t val = 0;
        
        while (producer_running.load(std::memory_order_relaxed)) {
            if (queue.push(val)) {
                val++;
            }
        }
    });

    // Main thread is the consumer
    // Pin to Core 1 (or second available)
    pin_thread_to_core(1);
    
    uint64_t val;
    for (auto _ : state) {
        // Spin until we get a value
        while (!queue.pop(val)) {
            benchmark::DoNotOptimize(val);
        }
    }

    producer_running.store(false, std::memory_order_relaxed);
    producer.join();
    
    state.SetBytesProcessed(state.iterations() * sizeof(uint64_t));
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_SPSCQueue_Throughput)->UseRealTime();

BENCHMARK_MAIN();
