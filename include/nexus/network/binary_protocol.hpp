#pragma once

#include "nexus/core/types.hpp"
#include <cstdint>

namespace nexus {

// Pragma pack ensures these structs have no padding, making them 
// exactly match the wire format for zero-copy casting.
#pragma pack(push, 1)

enum class MsgType : uint8_t {
    NewOrder = 1,
    CancelOrder = 2,
    ExecutionReport = 3,
    MarketDataL2 = 4
};

struct MsgHeader {
    uint16_t length; // Length of the entire message including header
    MsgType type;
    uint8_t version{1};
};

struct NewOrderMsg {
    MsgHeader header;
    OrderId client_order_id;
    Price price;
    Quantity qty;
    Side side;
    OrderType order_type;
};

struct CancelOrderMsg {
    MsgHeader header;
    OrderId client_order_id; // The ID of the cancel request itself
    OrderId target_order_id; // The ID of the order to cancel
};

struct ExecutionReportMsg {
    MsgHeader header;
    OrderId client_order_id;
    OrderId exec_id;
    Quantity executed_qty;
    Quantity leaves_qty; // Remaining quantity
    Price price;
    uint8_t exec_type; // 0=New, 1=Canceled, 2=Filled, 3=Rejected
};

#pragma pack(pop)

} // namespace nexus
