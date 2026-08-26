#include "nexus/oms/portfolio.hpp"
#include <cmath>

namespace nexus {

void Portfolio::process_fill(Side side, Price price, Quantity qty) {
    int64_t fill_qty = (side == Side::Buy) ? static_cast<int64_t>(qty) : -static_cast<int64_t>(qty);
    double fill_price = static_cast<double>(price);
    
    // Update Gross Notional
    gross_notional_ += static_cast<uint64_t>(qty) * price;
    
    // Check if we are opening, adding, reducing, or flipping a position
    if (net_position_ == 0) {
        // Opening new position
        average_open_price_ = fill_price;
    } else if ((net_position_ > 0 && fill_qty > 0) || (net_position_ < 0 && fill_qty < 0)) {
        // Adding to existing position (averaging price)
        double total_value = (std::abs(net_position_) * average_open_price_) + (std::abs(fill_qty) * fill_price);
        average_open_price_ = total_value / (std::abs(net_position_) + std::abs(fill_qty));
    } else {
        // Reducing or flipping position
        int64_t current_qty = std::abs(net_position_);
        int64_t reduce_qty = std::min(current_qty, std::abs(fill_qty));
        
        // Calculate realized PnL on the reduced portion
        if (net_position_ > 0) {
            // We were long, we sold
            realized_pnl_ += static_cast<int64_t>((fill_price - average_open_price_) * reduce_qty);
        } else {
            // We were short, we bought
            realized_pnl_ += static_cast<int64_t>((average_open_price_ - fill_price) * reduce_qty);
        }
        
        // If we flipped position (e.g. was long 10, sold 15 -> now short 5)
        if (std::abs(fill_qty) > current_qty) {
            average_open_price_ = fill_price; // New open price is the fill price
        } else if (std::abs(fill_qty) == current_qty) {
            average_open_price_ = 0.0; // Position is flat
        }
    }
    
    // Finally, update net position
    net_position_ += fill_qty;
}

void Portfolio::update_market_price(Price mid_price) {
    if (net_position_ == 0) {
        unrealized_pnl_ = 0;
        return;
    }
    
    double current_price = static_cast<double>(mid_price);
    if (net_position_ > 0) {
        unrealized_pnl_ = static_cast<int64_t>((current_price - average_open_price_) * net_position_);
    } else {
        unrealized_pnl_ = static_cast<int64_t>((average_open_price_ - current_price) * std::abs(net_position_));
    }
}

} // namespace nexus
