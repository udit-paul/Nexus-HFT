#pragma once

#include "nexus/core/types.hpp"
#include <cstdint>

namespace nexus {

class Portfolio {
public:
    Portfolio() = default;

    void process_fill(Side side, Price price, Quantity qty);
    void update_market_price(Price mid_price);

    int64_t get_net_position() const { return net_position_; }
    uint64_t get_gross_notional() const { return gross_notional_; }
    
    int64_t get_realized_pnl() const { return realized_pnl_; }
    int64_t get_unrealized_pnl() const { return unrealized_pnl_; }
    int64_t get_total_pnl() const { return realized_pnl_ + unrealized_pnl_; }

private:
    int64_t net_position_{0}; // Positive for long, negative for short
    uint64_t gross_notional_{0};
    
    // Average price of the current open position
    double average_open_price_{0.0};
    
    int64_t realized_pnl_{0};
    int64_t unrealized_pnl_{0};
};

} // namespace nexus
