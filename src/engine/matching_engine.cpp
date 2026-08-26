#include "nexus/engine/matching_engine.hpp"

namespace nexus {

SmallVector<Trade, 16> MatchingEngine::process_limit_order(OrderNode* incoming_order) {
    SmallVector<Trade, 16> trades;
    
    if (incoming_order->side == Side::Buy) {
        match_buy_order(incoming_order, trades);
    } else {
        match_sell_order(incoming_order, trades);
    }
    
    // If the order has remaining quantity, add it to the resting order book
    if (incoming_order->qty > 0) {
        book_.insert_order(incoming_order);
    }
    
    return trades;
}

void MatchingEngine::cancel_order(OrderNode* node) {
    book_.cancel_order(node);
}

void MatchingEngine::match_buy_order(OrderNode* incoming_order, SmallVector<Trade, 16>& trades) {
    while (incoming_order->qty > 0) {
        PriceLevel* best_ask = book_.get_best_ask();
        
        // No asks to match against, or the best ask is higher than the buy price limit
        if (!best_ask || best_ask->price > incoming_order->price) {
            break;
        }
        
        // Iterate through resting orders at the best ask price level
        OrderNode* resting_order = best_ask->orders.head();
        while (resting_order && incoming_order->qty > 0) {
            Quantity trade_qty = std::min(incoming_order->qty, resting_order->qty);
            
            // Record the trade
            trades.emplace_back(resting_order->id, incoming_order->id, best_ask->price, trade_qty);
            
            // Update quantities
            incoming_order->qty -= trade_qty;
            resting_order->qty -= trade_qty;
            best_ask->total_volume -= trade_qty;
            
            OrderNode* next_order = resting_order->next;
            
            // If the resting order is fully filled, remove it from the book
            if (resting_order->qty == 0) {
                best_ask->orders.remove(resting_order);
                // Note: The caller is responsible for deallocating `resting_order` from the MemoryPool,
                // perhaps via an ExecutionReport callback. For simplicity in this engine core, 
                // we just remove it from the intrusive list.
            }
            
            resting_order = next_order;
        }
        
        // Check if the price level is now empty and remove it if so
        book_.check_remove_level(best_ask, Side::Sell);
    }
}

void MatchingEngine::match_sell_order(OrderNode* incoming_order, SmallVector<Trade, 16>& trades) {
    while (incoming_order->qty > 0) {
        PriceLevel* best_bid = book_.get_best_bid();
        
        // No bids to match against, or the best bid is lower than the sell price limit
        if (!best_bid || best_bid->price < incoming_order->price) {
            break;
        }
        
        // Iterate through resting orders at the best bid price level
        OrderNode* resting_order = best_bid->orders.head();
        while (resting_order && incoming_order->qty > 0) {
            Quantity trade_qty = std::min(incoming_order->qty, resting_order->qty);
            
            // Record the trade
            trades.emplace_back(resting_order->id, incoming_order->id, best_bid->price, trade_qty);
            
            // Update quantities
            incoming_order->qty -= trade_qty;
            resting_order->qty -= trade_qty;
            best_bid->total_volume -= trade_qty;
            
            OrderNode* next_order = resting_order->next;
            
            // If the resting order is fully filled, remove it from the book
            if (resting_order->qty == 0) {
                best_bid->orders.remove(resting_order);
            }
            
            resting_order = next_order;
        }
        
        // Check if the price level is now empty and remove it if so
        book_.check_remove_level(best_bid, Side::Buy);
    }
}

} // namespace nexus
