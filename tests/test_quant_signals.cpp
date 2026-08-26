#include <gtest/gtest.h>
#include "nexus/quant/micro_price.hpp"
#include "nexus/quant/rolling_stats.hpp"

using namespace nexus;

TEST(SignalEngineTest, MicroPriceCalculation) {
    BookState state;
    state.best_bid = 100;
    state.bid_volume = 10;
    state.best_ask = 110;
    state.ask_volume = 10;
    
    // Equal volume, micro price should equal mid price (105)
    EXPECT_DOUBLE_EQ(SignalEngine::calculate_micro_price(state), 105.0);
    
    // Huge buying volume, micro price should shift closer to ask
    state.bid_volume = 90;
    state.ask_volume = 10;
    // (90 * 110 + 10 * 100) / 100 = (9900 + 1000) / 100 = 10900 / 100 = 109
    EXPECT_DOUBLE_EQ(SignalEngine::calculate_micro_price(state), 109.0);
}

TEST(SignalEngineTest, OrderFlowImbalance) {
    SignalEngine engine;
    
    BookState s1{100, 10, 110, 10};
    EXPECT_DOUBLE_EQ(engine.update_and_get_ofi(s1), 0.0); // First tick
    
    // Bid volume increases, Ask volume stays same (positive OFI)
    BookState s2{100, 15, 110, 10};
    EXPECT_DOUBLE_EQ(engine.update_and_get_ofi(s2), 5.0);
    
    // Bid price drops (negative OFI)
    BookState s3{99, 20, 110, 10};
    EXPECT_DOUBLE_EQ(engine.update_and_get_ofi(s3), -15.0); // 0 - 15 - (10 - 10) = -15
    
    // Ask price drops (negative OFI as it's selling pressure)
    BookState s4{99, 20, 109, 5};
    EXPECT_DOUBLE_EQ(engine.update_and_get_ofi(s4), -5.0); // 0 - (0) - 5
}

TEST(RollingStatsTest, VarianceAndMean) {
    RollingStats stats(3); // Window size 3
    
    stats.add(10.0);
    EXPECT_DOUBLE_EQ(stats.mean(), 10.0);
    EXPECT_DOUBLE_EQ(stats.variance(), 0.0);
    EXPECT_FALSE(stats.is_ready());
    
    stats.add(20.0);
    EXPECT_DOUBLE_EQ(stats.mean(), 15.0); // (10+20)/2 = 15
    EXPECT_DOUBLE_EQ(stats.variance(), 50.0); // ((10-15)^2 + (20-15)^2) / 1 = 50
    EXPECT_FALSE(stats.is_ready());
    
    stats.add(30.0);
    EXPECT_DOUBLE_EQ(stats.mean(), 20.0);
    EXPECT_DOUBLE_EQ(stats.variance(), 100.0);
    EXPECT_TRUE(stats.is_ready());
    
    // Slides window, removes 10.0, adds 40.0
    // Window is now [20.0, 30.0, 40.0]
    stats.add(40.0);
    EXPECT_DOUBLE_EQ(stats.mean(), 30.0);
    EXPECT_DOUBLE_EQ(stats.variance(), 100.0);
}
