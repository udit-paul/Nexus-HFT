#include <gtest/gtest.h>
#include "nexus/risk/risk_engine.hpp"

using namespace nexus;

class RiskEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        RiskLimits limits;
        limits.max_order_qty = 1000;
        limits.max_notional = 100000;
        limits.max_price_deviation_pct = 5;
        
        engine = std::make_unique<RiskEngine>(limits);
        RiskEngine::reset_kill_switch();
    }
    
    std::unique_ptr<RiskEngine> engine;
};

TEST_F(RiskEngineTest, NormalOrderPasses) {
    EXPECT_EQ(engine->check_order(100, 50, Side::Buy, 100), RejectReason::None);
}

TEST_F(RiskEngineTest, MaxOrderQtyExceeded) {
    EXPECT_EQ(engine->check_order(100, 1001, Side::Buy, 100), RejectReason::MaxOrderQtyExceeded);
}

TEST_F(RiskEngineTest, MaxNotionalExceeded) {
    // 101 * 1000 = 101000 (Max is 100000)
    EXPECT_EQ(engine->check_order(101, 1000, Side::Buy, 100), RejectReason::MaxNotionalExceeded);
}

TEST_F(RiskEngineTest, PriceCollarExceededBuy) {
    // Mid price is 100. Max deviation is 5%. Max buy price is 105.
    EXPECT_EQ(engine->check_order(106, 10, Side::Buy, 100), RejectReason::PriceCollarExceeded);
    EXPECT_EQ(engine->check_order(105, 10, Side::Buy, 100), RejectReason::None);
}

TEST_F(RiskEngineTest, PriceCollarExceededSell) {
    // Mid price is 100. Max deviation is 5%. Min sell price is 95.
    EXPECT_EQ(engine->check_order(94, 10, Side::Sell, 100), RejectReason::PriceCollarExceeded);
    EXPECT_EQ(engine->check_order(95, 10, Side::Sell, 100), RejectReason::None);
}

TEST_F(RiskEngineTest, KillSwitch) {
    EXPECT_EQ(engine->check_order(100, 50, Side::Buy, 100), RejectReason::None);
    
    RiskEngine::trip_kill_switch();
    EXPECT_TRUE(RiskEngine::is_kill_switch_active());
    
    EXPECT_EQ(engine->check_order(100, 50, Side::Buy, 100), RejectReason::KillSwitchActive);
    
    RiskEngine::reset_kill_switch();
    EXPECT_EQ(engine->check_order(100, 50, Side::Buy, 100), RejectReason::None);
}
