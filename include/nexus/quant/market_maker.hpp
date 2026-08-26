#pragma once

#include "nexus/core/types.hpp"
#include <cmath>

namespace nexus {

struct AvellanedaStoikovParams {
    double gamma{0.1}; // Inventory risk aversion parameter
    double sigma{1.0}; // Volatility (can be dynamically updated)
    double k{1.5};     // Order book density / liquidity parameter
    double T{1.0};     // Total time horizon
};

struct Quotes {
    Price bid;
    Price ask;
};

class AvellanedaStoikov {
public:
    explicit AvellanedaStoikov(const AvellanedaStoikovParams& params) 
        : params_(params) {}

    // Calculate the reservation (fair) price skewed by inventory
    double calculate_reservation_price(double mid_price, int64_t inventory, double t) const;
    
    // Calculate optimal spreads around the reservation price
    double calculate_optimal_spread(double t) const;

    // Generate actual bid/ask quotes based on current state
    Quotes get_quotes(double mid_price, int64_t inventory, double t) const;

    void update_volatility(double new_sigma) {
        params_.sigma = new_sigma;
    }

private:
    AvellanedaStoikovParams params_;
};

} // namespace nexus
