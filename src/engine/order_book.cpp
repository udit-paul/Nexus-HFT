#include "nexus/core/order_book.hpp"

namespace nexus {

// ── OrderBook constructor ──────────────────────────────────────────────────────
//
// Initialises the two arena states over the embedded char buffers, then
// constructs the maps with PoolAllocator instances pointing into those arenas.
//
// The arena's slot_size is determined from sizeof(BidMap::node_type) at the
// std::map level, but since that type is implementation-defined, we pass the
// MAP_SLOT_MAX_BYTES budget (256 bytes) which is guaranteed to be ≥ any
// real node size on all supported platforms.
//
OrderBook::OrderBook()
    : bid_arena_state_{},
      ask_arena_state_{},
      bids_(std::greater<Price>{},
            PoolAllocator<std::pair<const Price, PriceLevel>>(&bid_arena_state_)),
      asks_(std::less<Price>{},
            PoolAllocator<std::pair<const Price, PriceLevel>>(&ask_arena_state_))
{
    bid_arena_state_.init(bid_arena_buf_, sizeof(bid_arena_buf_),
                          MAP_SLOT_MAX_BYTES, MAP_SLOT_ALIGN);
    ask_arena_state_.init(ask_arena_buf_, sizeof(ask_arena_buf_),
                          MAP_SLOT_MAX_BYTES, MAP_SLOT_ALIGN);
}

// ── insert_order ──────────────────────────────────────────────────────────────

void OrderBook::insert_order(OrderNode* node) {
    if (node->side == Side::Buy) {
        auto it = bids_.find(node->price);
        if (it == bids_.end()) {
            it = bids_.emplace(node->price, PriceLevel(node->price)).first;
        }
        it->second.orders.push_back(node);
        it->second.total_volume += node->qty;
    } else {
        auto it = asks_.find(node->price);
        if (it == asks_.end()) {
            it = asks_.emplace(node->price, PriceLevel(node->price)).first;
        }
        it->second.orders.push_back(node);
        it->second.total_volume += node->qty;
    }
}

// ── cancel_order ──────────────────────────────────────────────────────────────

void OrderBook::cancel_order(OrderNode* node) {
    if (node->side == Side::Buy) {
        auto it = bids_.find(node->price);
        if (it != bids_.end()) {
            it->second.orders.remove(node);
            it->second.total_volume -= node->qty;
            check_remove_level(&it->second, Side::Buy);
        }
    } else {
        auto it = asks_.find(node->price);
        if (it != asks_.end()) {
            it->second.orders.remove(node);
            it->second.total_volume -= node->qty;
            check_remove_level(&it->second, Side::Sell);
        }
    }
}

// ── reduce_order ──────────────────────────────────────────────────────────────

void OrderBook::reduce_order(OrderNode* node, Quantity new_qty) {
    if (new_qty >= node->qty) return; // Only reductions are allowed to maintain time priority

    Quantity diff = node->qty - new_qty;
    node->qty = new_qty;

    if (node->side == Side::Buy) {
        auto it = bids_.find(node->price);
        if (it != bids_.end()) {
            it->second.total_volume -= diff;
        }
    } else {
        auto it = asks_.find(node->price);
        if (it != asks_.end()) {
            it->second.total_volume -= diff;
        }
    }
}

// ── get_best_bid / get_best_ask ────────────────────────────────────────────────

PriceLevel* OrderBook::get_best_bid() {
    if (bids_.empty()) return nullptr;
    return &bids_.begin()->second;
}

PriceLevel* OrderBook::get_best_ask() {
    if (asks_.empty()) return nullptr;
    return &asks_.begin()->second;
}

// ── check_remove_level ────────────────────────────────────────────────────────

void OrderBook::check_remove_level(PriceLevel* level, Side side) {
    if (level->orders.empty()) {
        if (side == Side::Buy) {
            bids_.erase(level->price);
        } else {
            asks_.erase(level->price);
        }
    }
}

} // namespace nexus
