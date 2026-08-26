#include <gtest/gtest.h>
#include "nexus/quant/market_maker.hpp"

using namespace nexus;

TEST(AvellanedaStoikovTest, ReservationPriceShift) {
    AvellanedaStoikovParams params;
    params.gamma = 0.1;
    params.sigma = 1.0;
    params.T = 1.0;
    
    AvellanedaStoikov mm(params);
    double mid_price = 100.0;
    double t = 0.0;
    
    // Flat inventory: Reservation price == Mid price
    EXPECT_DOUBLE_EQ(mm.calculate_reservation_price(mid_price, 0, t), 100.0);
    
    // Long inventory (+10): Reservation price drops to incentivize selling and discourage buying
    // R = 100 - (10 * 0.1 * 1.0^2 * 1.0) = 100 - 1.0 = 99.0
    EXPECT_DOUBLE_EQ(mm.calculate_reservation_price(mid_price, 10, t), 99.0);
    
    // Short inventory (-10): Reservation price rises to incentivize buying and discourage selling
    // R = 100 - (-10 * 0.1 * 1.0^2 * 1.0) = 100 + 1.0 = 101.0
    EXPECT_DOUBLE_EQ(mm.calculate_reservation_price(mid_price, -10, t), 101.0);
}

TEST(AvellanedaStoikovTest, OptimalQuotes) {
    AvellanedaStoikovParams params;
    params.gamma = 0.1;
    params.sigma = 2.0; // Higher volatility
    params.T = 1.0;
    params.k = 1.5;
    
    AvellanedaStoikov mm(params);
    
    double mid_price = 1000.0;
    double t = 0.5; // Halfway through trading session
    
    // Test flat inventory quotes
    Quotes flat_quotes = mm.get_quotes(mid_price, 0, t);
    
    // Test long inventory quotes
    Quotes long_quotes = mm.get_quotes(mid_price, 50, t);
    
    // With long inventory, both bid and ask should be lower than flat inventory
    // to attract buyers (who buy at our ask) and repel sellers (who sell to our bid)
    EXPECT_LT(long_quotes.bid, flat_quotes.bid);
    EXPECT_LT(long_quotes.ask, flat_quotes.ask);
    
    // Test short inventory quotes
    Quotes short_quotes = mm.get_quotes(mid_price, -50, t);
    
    // With short inventory, both bid and ask should be higher than flat inventory
    EXPECT_GT(short_quotes.bid, flat_quotes.bid);
    EXPECT_GT(short_quotes.ask, flat_quotes.ask);
}
