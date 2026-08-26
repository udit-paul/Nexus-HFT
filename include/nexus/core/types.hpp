#pragma once

#include <cstdint>
#include <string>
#include <limits>

namespace nexus {

// Fixed point integer for price to avoid float rounding issues
// Price is typically scaled by 10000 or similar (e.g. 101.50 -> 1015000)
using Price = uint64_t;
using Quantity = uint64_t;
using OrderId = uint64_t;

enum class Side : uint8_t {
    Buy = 0,
    Sell = 1
};

enum class OrderType : uint8_t {
    Limit = 0,
    Market = 1
};

enum class TimeInForce : uint8_t {
    GTC = 0, // Good Till Cancel
    IOC = 1, // Immediate Or Cancel
    FOK = 2  // Fill Or Kill
};

constexpr Price MAX_PRICE = std::numeric_limits<Price>::max();
constexpr Price MIN_PRICE = 0;

} // namespace nexus
