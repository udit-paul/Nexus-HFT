#include "nexus/quant/micro_price.hpp"

namespace nexus {

double SignalEngine::calculate_micro_price(const BookState& current) {
    if (current.bid_volume + current.ask_volume == 0) return 0.0;
    
    // P_micro = (V_bid * P_ask + V_ask * P_bid) / (V_bid + V_ask)
    double p_bid = static_cast<double>(current.best_bid);
    double p_ask = static_cast<double>(current.best_ask);
    double v_bid = static_cast<double>(current.bid_volume);
    double v_ask = static_cast<double>(current.ask_volume);
    
    return (v_bid * p_ask + v_ask * p_bid) / (v_bid + v_ask);
}

double SignalEngine::update_and_get_ofi(const BookState& current) {
    if (!has_prev_) {
        prev_state_ = current;
        has_prev_ = true;
        return 0.0;
    }
    
    // OFI calculation
    double ofi = 0.0;
    
    // Bid side (buying pressure)
    if (current.best_bid > prev_state_.best_bid) {
        ofi += static_cast<double>(current.bid_volume);
    } else if (current.best_bid == prev_state_.best_bid) {
        ofi += static_cast<double>(current.bid_volume) - static_cast<double>(prev_state_.bid_volume);
    } else {
        ofi -= static_cast<double>(prev_state_.bid_volume);
    }
    
    // Ask side (selling pressure)
    if (current.best_ask < prev_state_.best_ask) {
        ofi -= static_cast<double>(current.ask_volume);
    } else if (current.best_ask == prev_state_.best_ask) {
        ofi -= static_cast<double>(current.ask_volume) - static_cast<double>(prev_state_.ask_volume);
    } else {
        ofi += static_cast<double>(prev_state_.ask_volume);
    }
    
    prev_state_ = current;
    return ofi;
}

} // namespace nexus
