#pragma once

#include "nexus/core/types.hpp"
#include "nexus/core/order_book.hpp"
#include "nexus/core/small_vector.hpp"
#include "nexus/engine/trade.hpp"

namespace nexus {

class MatchingEngine {
public:
    MatchingEngine() = default;

    // Process a new limit order, returning a list of trades generated.
    // The order node is allocated externally (e.g., via MemoryPool).
    // If the order is not fully filled, it will be added to the order book.
    // Returns SmallVector<Trade, 16> — zero heap allocation for ≤16 trades.
    SmallVector<Trade, 16> process_limit_order(OrderNode* incoming_order);

    // Cancel an existing order
    void cancel_order(OrderNode* node);

    // Get the underlying order book
    OrderBook& get_book() { return book_; }

private:
    OrderBook book_;

    void match_buy_order(OrderNode* incoming_order, SmallVector<Trade, 16>& trades);
    void match_sell_order(OrderNode* incoming_order, SmallVector<Trade, 16>& trades);
};

} // namespace nexus
