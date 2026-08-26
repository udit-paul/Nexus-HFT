#pragma once

#include "nexus/core/types.hpp"
#include <cstdint>

namespace nexus {

struct BookState {
    Price best_bid{0};
    Quantity bid_volume{0};
    Price best_ask{0};
    Quantity ask_volume{0};
};

class SignalEngine {
public:
    SignalEngine() = default;

    // Calculates volume-weighted micro price
    // P_micro = (V_bid * P_ask + V_ask * P_bid) / (V_bid + V_ask)
    static double calculate_micro_price(const BookState& current);

    // Updates state and calculates Order Flow Imbalance (OFI)
    // Returns the OFI value for the current tick
    double update_and_get_ofi(const BookState& current);

private:
    BookState prev_state_;
    bool has_prev_{false};
};

} // namespace nexus
