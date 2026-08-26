#pragma once

#include "nexus/core/types.hpp"
#include "nexus/core/intrusive_list.hpp"
#include "nexus/core/pool_allocator.hpp"
#include <map>
#include <unordered_map>
#include <cstdint>

namespace nexus {

// ── Arena sizing ─────────────────────────────────────────────────────────────
//
// std::map allocates one internal node (std::_Rb_tree_node<value_type>) per
// price level. On MSVC x64, this node is typically 64 bytes (colour + 3
// pointers + pair<const Key, Value>). We conservatively budget 256 bytes
// per slot and pre-allocate space for 512 price levels per side.
//
// Total arena footprint per side: 512 × 256 = 128 KB (fits in L2 cache).
//
static constexpr std::size_t MAX_PRICE_LEVELS   = 512;
static constexpr std::size_t MAP_SLOT_MAX_BYTES  = 256;
static constexpr std::size_t MAP_SLOT_ALIGN      = alignof(std::max_align_t);
static constexpr std::size_t ARENA_BYTES         =
    MAX_PRICE_LEVELS * MAP_SLOT_MAX_BYTES;

// ── Forward declaration of OrderNode ─────────────────────────────────────────

struct alignas(64) OrderNode {
    OrderId  id;
    Price    price;
    Quantity qty;
    Side     side;

    // Intrusive list pointers (null when node is not resting in the book)
    OrderNode* prev{nullptr};
    OrderNode* next{nullptr};

    OrderNode() = default;

    OrderNode(OrderId id_, Price price_, Quantity qty_, Side side_)
        : id(id_), price(price_), qty(qty_), side(side_) {}
};

inline void IntrusiveList::push_back(OrderNode* node) {
    if (!head_) {
        head_ = tail_ = node;
        node->prev = node->next = nullptr;
    } else {
        tail_->next = node;
        node->prev  = tail_;
        node->next  = nullptr;
        tail_       = node;
    }
}

inline void IntrusiveList::remove(OrderNode* node) {
    if (node->prev) {
        node->prev->next = node->next;
    } else {
        head_ = node->next;
    }
    if (node->next) {
        node->next->prev = node->prev;
    } else {
        tail_ = node->prev;
    }
    node->prev = nullptr;
    node->next = nullptr;
}

// A single price level holding all orders at this price
struct PriceLevel {
    Price         price;
    Quantity      total_volume{0};
    IntrusiveList orders;

    explicit PriceLevel(Price p) : price(p) {}
};

// ── Limit Order Book (LOB) ────────────────────────────────────────────────────

class OrderBook {
public:
    // Pool-backed map types
    using BidMap = std::map<Price, PriceLevel, std::greater<Price>,
                            PoolAllocator<std::pair<const Price, PriceLevel>>>;
    using AskMap = std::map<Price, PriceLevel, std::less<Price>,
                            PoolAllocator<std::pair<const Price, PriceLevel>>>;

    OrderBook();

    // Insert a resting limit order into the book
    void insert_order(OrderNode* node);

    // Cancel an order from the book (O(1))
    void cancel_order(OrderNode* node);

    // Modify the quantity of an order (if reducing, keeps time priority)
    void reduce_order(OrderNode* node, Quantity new_qty);

    // Helpers to access the top of book
    PriceLevel* get_best_bid();
    PriceLevel* get_best_ask();

    // Get full depth (for display/web; not called on the hot path)
    const BidMap& get_bids() const { return bids_; }
    const AskMap& get_asks() const { return asks_; }

    // Remove empty price levels (called after matching or canceling the last order at a level)
    void check_remove_level(PriceLevel* level, Side side);

private:
    // ── Pre-allocated arenas ────────────────────────────────────────────────
    // Stored as members so they live for the lifetime of OrderBook.
    // Aligned to a cache line to avoid false sharing with adjacent data.
    alignas(64) char bid_arena_buf_[ARENA_BYTES];
    alignas(64) char ask_arena_buf_[ARENA_BYTES];

    // ArenaState objects manage the freelist over the raw arena buffers.
    detail::ArenaState bid_arena_state_;
    detail::ArenaState ask_arena_state_;

    // ── Pool-backed maps ────────────────────────────────────────────────────
    // Bids: sorted descending (highest price first)
    BidMap bids_;
    // Asks: sorted ascending (lowest price first)
    AskMap asks_;
};

} // namespace nexus
