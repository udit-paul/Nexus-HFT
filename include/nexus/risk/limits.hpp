#pragma once

#include "nexus/core/types.hpp"

namespace nexus {

struct RiskLimits {
    // Maximum allowable quantity for a single order (Fat-Finger check)
    Quantity max_order_qty{10000};
    
    // Maximum allowable notional value (price * qty) for a single order
    uint64_t max_notional{1000000000}; // e.g. 1B if price is in pennies
    
    // Price collar bounds relative to current mid price or reference price
    // e.g. 5 means 5% (handled as integer division based on fixed point scaling)
    uint8_t max_price_deviation_pct{5}; 
};

enum class RejectReason : uint8_t {
    None = 0,
    MaxOrderQtyExceeded = 1,
    MaxNotionalExceeded = 2,
    PriceCollarExceeded = 3,
    KillSwitchActive = 4,
    UnknownError = 5
};

} // namespace nexus
