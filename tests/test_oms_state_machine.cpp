#include <gtest/gtest.h>
#include "nexus/oms/portfolio.hpp"

using namespace nexus;

TEST(PortfolioTest, NetPositionUpdates) {
    Portfolio portfolio;
    
    EXPECT_EQ(portfolio.get_net_position(), 0);
    
    portfolio.process_fill(Side::Buy, 100, 10);
    EXPECT_EQ(portfolio.get_net_position(), 10);
    
    portfolio.process_fill(Side::Buy, 100, 5);
    EXPECT_EQ(portfolio.get_net_position(), 15);
    
    portfolio.process_fill(Side::Sell, 100, 15);
    EXPECT_EQ(portfolio.get_net_position(), 0);
    
    portfolio.process_fill(Side::Sell, 100, 20);
    EXPECT_EQ(portfolio.get_net_position(), -20);
}

TEST(PortfolioTest, RealizedPnL) {
    Portfolio portfolio;
    
    // Buy 10 @ 100
    portfolio.process_fill(Side::Buy, 100, 10);
    EXPECT_EQ(portfolio.get_realized_pnl(), 0);
    
    // Sell 5 @ 110 (Profit of 10 * 5 = 50)
    portfolio.process_fill(Side::Sell, 110, 5);
    EXPECT_EQ(portfolio.get_realized_pnl(), 50);
    EXPECT_EQ(portfolio.get_net_position(), 5);
    
    // Sell 5 @ 90 (Loss of 10 * 5 = -50)
    portfolio.process_fill(Side::Sell, 90, 5);
    EXPECT_EQ(portfolio.get_realized_pnl(), 0); // 50 - 50
    EXPECT_EQ(portfolio.get_net_position(), 0);
    
    // Short 10 @ 100
    portfolio.process_fill(Side::Sell, 100, 10);
    
    // Buy back 10 @ 90 (Profit of 10 * 10 = 100)
    portfolio.process_fill(Side::Buy, 90, 10);
    EXPECT_EQ(portfolio.get_realized_pnl(), 100);
}

TEST(PortfolioTest, UnrealizedPnL) {
    Portfolio portfolio;
    
    // Buy 10 @ 100
    portfolio.process_fill(Side::Buy, 100, 10);
    
    portfolio.update_market_price(110);
    EXPECT_EQ(portfolio.get_unrealized_pnl(), 100); // 10 * 10
    
    portfolio.update_market_price(90);
    EXPECT_EQ(portfolio.get_unrealized_pnl(), -100); // -10 * 10
    
    // Sell 10 @ 110 (Flats position)
    portfolio.process_fill(Side::Sell, 110, 10);
    portfolio.update_market_price(110);
    EXPECT_EQ(portfolio.get_unrealized_pnl(), 0);
}
