#include <gtest/gtest.h>
#include "nexus/engine/matching_engine.hpp"
#include "nexus/core/memory_pool.hpp"

using namespace nexus;

class MatchingEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        pool = std::make_unique<MemoryPool<OrderNode>>(1000);
    }
    
    std::unique_ptr<MemoryPool<OrderNode>> pool;
    MatchingEngine engine;
};

TEST_F(MatchingEngineTest, ExactMatch) {
    OrderNode* sell = pool->allocate(1, 100, 10, Side::Sell);
    engine.process_limit_order(sell);
    
    OrderNode* buy = pool->allocate(2, 100, 10, Side::Buy);
    auto trades = engine.process_limit_order(buy);
    
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].price, 100);
    EXPECT_EQ(trades[0].qty, 10);
    EXPECT_EQ(trades[0].maker_order_id, 1);
    EXPECT_EQ(trades[0].taker_order_id, 2);
    
    EXPECT_EQ(engine.get_book().get_best_bid(), nullptr);
    EXPECT_EQ(engine.get_book().get_best_ask(), nullptr);
}

TEST_F(MatchingEngineTest, PartialFill) {
    OrderNode* sell = pool->allocate(1, 100, 20, Side::Sell);
    engine.process_limit_order(sell);
    
    OrderNode* buy = pool->allocate(2, 100, 15, Side::Buy);
    auto trades = engine.process_limit_order(buy);
    
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].qty, 15);
    
    PriceLevel* best_ask = engine.get_book().get_best_ask();
    ASSERT_NE(best_ask, nullptr);
    EXPECT_EQ(best_ask->total_volume, 5); // 5 left on the book
}

TEST_F(MatchingEngineTest, TimePriority) {
    OrderNode* sell1 = pool->allocate(1, 100, 10, Side::Sell); // First in
    OrderNode* sell2 = pool->allocate(2, 100, 10, Side::Sell); // Second in
    
    engine.process_limit_order(sell1);
    engine.process_limit_order(sell2);
    
    OrderNode* buy = pool->allocate(3, 100, 5, Side::Buy);
    auto trades = engine.process_limit_order(buy);
    
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].maker_order_id, 1); // Should match with sell1
    
    PriceLevel* best_ask = engine.get_book().get_best_ask();
    ASSERT_NE(best_ask, nullptr);
    EXPECT_EQ(best_ask->total_volume, 15); // 5 left on sell1 + 10 on sell2
}
