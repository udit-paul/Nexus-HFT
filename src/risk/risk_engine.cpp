#include "nexus/risk/risk_engine.hpp"

namespace nexus {

std::atomic<bool> RiskEngine::global_kill_switch_{false};

RejectReason RiskEngine::check_order(Price price, Quantity qty, Side side, Price reference_price) const {
    // 1. Check Global Kill Switch first (relaxed load is extremely fast)
    if (global_kill_switch_.load(std::memory_order_relaxed)) {
        return RejectReason::KillSwitchActive;
    }

    // 2. Fat-Finger Size Check
    if (qty > limits_.max_order_qty) {
        return RejectReason::MaxOrderQtyExceeded;
    }

    // 3. Max Notional Check
    uint64_t notional = static_cast<uint64_t>(price) * static_cast<uint64_t>(qty);
    if (notional > limits_.max_notional) {
        return RejectReason::MaxNotionalExceeded;
    }

    // 4. Price Collar Check (only if we have a valid reference price, e.g., Mid Price)
    if (reference_price > 0) {
        Price max_deviation = (reference_price * limits_.max_price_deviation_pct) / 100;
        
        if (side == Side::Buy) {
            // Can't buy too high
            if (price > reference_price + max_deviation) {
                return RejectReason::PriceCollarExceeded;
            }
        } else {
            // Can't sell too low
            if (price < reference_price - max_deviation) {
                return RejectReason::PriceCollarExceeded;
            }
        }
    }

    return RejectReason::None;
}

void RiskEngine::trip_kill_switch() {
    global_kill_switch_.store(true, std::memory_order_release);
}

void RiskEngine::reset_kill_switch() {
    global_kill_switch_.store(false, std::memory_order_release);
}

bool RiskEngine::is_kill_switch_active() {
    return global_kill_switch_.load(std::memory_order_acquire);
}

} // namespace nexus
