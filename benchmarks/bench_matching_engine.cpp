/**
 * bench_matching_engine.cpp
 *
 * Google Benchmark suite for the Nexus-HFT matching engine hot path.
 * Measures three representative scenarios:
 *
 *   1. BM_Insert_RestingOrder   — A limit order that does NOT match (pure insertion).
 *      This is the baseline: just a map lookup + intrusive list push.
 *
 *   2. BM_Cross_SingleTrade     — A crossing order that generates exactly one trade
 *      against a single resting order at the same price.
 *      This hits the SmallVector emplace_back path (should be zero-alloc).
 *
 *   3. BM_Sweep_FiveLevels      — A large buy order that sweeps through five
 *      resting ask price levels, generating multiple trades.
 *      Tests the map erase + check_remove_level path under load.
 *
 * Run with: ./bench_matching_engine --benchmark_format=json
 * Compare before/after optimization with: --benchmark_filter=BM_
 */

#include <benchmark/benchmark.h>
#include "nexus/engine/matching_engine.hpp"
#include "nexus/core/memory_pool.hpp"

using namespace nexus;

// ── Shared fixture helpers ────────────────────────────────────────────────────

static constexpr std::size_t POOL_CAPACITY = 1'000'000;

// Prices in fixed-point cents (e.g. 10000 = $100.00)
static constexpr Price BASE_PRICE = 10000;

// ── BM_Insert_RestingOrder ────────────────────────────────────────────────────

/**
 * Measures the cost of inserting a resting limit order that finds no match.
 * Each iteration: allocate node → process_limit_order (no trades) → cancel order.
 * This gives us the pure book-insertion latency.
 */
static void BM_Insert_RestingOrder(benchmark::State& state) {
    MemoryPool<OrderNode> pool(POOL_CAPACITY);

    for (auto _ : state) {
        MatchingEngine engine;

        // Place a sell at BASE_PRICE
        OrderNode* sell = pool.allocate(1, BASE_PRICE, 100, Side::Sell);
        benchmark::DoNotOptimize(engine.process_limit_order(sell));

        // Place a buy well below BASE_PRICE (no match — rests in book)
        OrderNode* buy = pool.allocate(2, BASE_PRICE - 100, 100, Side::Buy);
        benchmark::DoNotOptimize(engine.process_limit_order(buy));

        // Cleanup: cancel both resting orders and return to pool
        engine.cancel_order(sell);
        engine.cancel_order(buy);
        pool.deallocate(sell);
        pool.deallocate(buy);
    }

    state.SetLabel("insert_resting");
    state.SetItemsProcessed(state.iterations() * 2);
}
BENCHMARK(BM_Insert_RestingOrder)->UseRealTime()->Repetitions(5);

// ── BM_Cross_SingleTrade ──────────────────────────────────────────────────────

/**
 * Pre-populates the book with a resting sell, then measures the cost of
 * sending a matching buy (generates exactly 1 trade, fully consumes the resting order).
 * This is the most common real-world scenario.
 */
static void BM_Cross_SingleTrade(benchmark::State& state) {
    MemoryPool<OrderNode> pool(POOL_CAPACITY);
    OrderId id = 1;

    for (auto _ : state) {
        // Re-create engine per iteration so book is always clean
        MatchingEngine engine;

        // Rest a sell at BASE_PRICE
        OrderNode* sell = pool.allocate(id++, BASE_PRICE, 10, Side::Sell);
        engine.process_limit_order(sell);   // rests in book

        // Measure: incoming buy crosses the spread
        state.ResumeTiming();
        OrderNode* buy = pool.allocate(id++, BASE_PRICE, 10, Side::Buy);
        auto trades = engine.process_limit_order(buy);
        benchmark::DoNotOptimize(trades);
        state.PauseTiming();

        // sell was consumed by the match; buy is also consumed. Return buy to pool.
        // (sell was removed from book internally by the engine)
        pool.deallocate(buy);
        pool.deallocate(sell);
    }

    state.SetLabel("cross_1_trade");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Cross_SingleTrade)->UseRealTime()->Repetitions(5);

// ── BM_Sweep_FiveLevels ───────────────────────────────────────────────────────

/**
 * Pre-populates 5 distinct price levels on the ask side, each with one order.
 * Measures the cost of a large buy order that sweeps through all 5 levels,
 * generating 5 trades and triggering 5 map erases.
 */
static void BM_Sweep_FiveLevels(benchmark::State& state) {
    MemoryPool<OrderNode> pool(POOL_CAPACITY);
    OrderId id = 1;

    static constexpr int NUM_LEVELS = 5;
    static constexpr Quantity QTY_PER_LEVEL = 10;

    for (auto _ : state) {
        MatchingEngine engine;

        // Rest one sell at each of 5 consecutive price levels
        OrderNode* sells[NUM_LEVELS];
        for (int i = 0; i < NUM_LEVELS; ++i) {
            sells[i] = pool.allocate(id++, BASE_PRICE + static_cast<Price>(i * 10),
                                     QTY_PER_LEVEL, Side::Sell);
            engine.process_limit_order(sells[i]);
        }

        // Measure: incoming buy sweeps all 5 levels
        state.ResumeTiming();
        // Buy price is high enough to cross all levels; qty exactly fills all 5
        OrderNode* buy = pool.allocate(id++,
                                       BASE_PRICE + static_cast<Price>((NUM_LEVELS - 1) * 10),
                                       QTY_PER_LEVEL * NUM_LEVELS,
                                       Side::Buy);
        auto trades = engine.process_limit_order(buy);
        benchmark::DoNotOptimize(trades);
        state.PauseTiming();

        pool.deallocate(buy);
        for (int i = 0; i < NUM_LEVELS; ++i) {
            pool.deallocate(sells[i]);
        }
    }

    state.SetLabel("sweep_5_levels");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Sweep_FiveLevels)->UseRealTime()->Repetitions(5);

// ── BM_HotPath_Sustained ──────────────────────────────────────────────────────

/**
 * Sustained throughput benchmark: alternates between resting a sell and
 * crossing it with a buy, 1M times. Measures the steady-state throughput
 * of the engine under realistic load (book never empty, never overflowing).
 * Reports throughput in orders/second.
 */
static void BM_HotPath_Sustained(benchmark::State& state) {
    MemoryPool<OrderNode> pool(POOL_CAPACITY);

    for (auto _ : state) {
        MatchingEngine engine;
        OrderId id = 1;

        // Sustain the book with a resting sell so crossing orders always match
        OrderNode* resting = pool.allocate(id++, BASE_PRICE, 1'000'000, Side::Sell);
        engine.process_limit_order(resting);

        // Cross it 1000 times with tiny buys (qty=1 each)
        for (int i = 0; i < 1000; ++i) {
            OrderNode* buy = pool.allocate(id++, BASE_PRICE, 1, Side::Buy);
            auto trades = engine.process_limit_order(buy);
            benchmark::DoNotOptimize(trades);
            pool.deallocate(buy);
        }
        pool.deallocate(resting);
    }

    state.SetLabel("sustained_throughput");
    state.SetItemsProcessed(state.iterations() * 1000);
}
BENCHMARK(BM_HotPath_Sustained)->UseRealTime()->Repetitions(3);

BENCHMARK_MAIN();
