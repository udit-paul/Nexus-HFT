#pragma once

#include "nexus/core/types.hpp"

namespace nexus {

struct Trade {
    OrderId maker_order_id;
    OrderId taker_order_id;
    Price price;
    Quantity qty;

    Trade(OrderId m, OrderId t, Price p, Quantity q)
        : maker_order_id(m), taker_order_id(t), price(p), qty(q) {}
};

// In a real system, ExecutionReport would include more details like 
// remaining quantity, ExecType (New, Canceled, Filled, Rejected), etc.
struct ExecutionReport {
    OrderId order_id;
    Quantity executed_qty;
    Price execution_price;
    // ... other fields
};

} // namespace nexus
