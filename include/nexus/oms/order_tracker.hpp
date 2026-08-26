#pragma once

#include "nexus/core/types.hpp"

namespace nexus {

enum class OrderState : uint8_t {
    PendingNew = 0,
    New = 1,
    PartiallyFilled = 2,
    Filled = 3,
    PendingCancel = 4,
    Canceled = 5,
    Rejected = 6
};

// Represents the client/internal view of an order
struct OrderTracker {
    OrderId order_id;
    Side side;
    Price price;
    Quantity original_qty;
    Quantity executed_qty{0};
    OrderState state{OrderState::PendingNew};
    
    OrderTracker(OrderId id, Side s, Price p, Quantity q)
        : order_id(id), side(s), price(p), original_qty(q) {}
        
    Quantity remaining_qty() const {
        return original_qty - executed_qty;
    }
};

} // namespace nexus
