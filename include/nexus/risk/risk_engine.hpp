#pragma once

#include "nexus/core/types.hpp"
#include "nexus/risk/limits.hpp"
#include <atomic>

namespace nexus {

class RiskEngine {
public:
    explicit RiskEngine(const RiskLimits& limits) : limits_(limits) {}

    // Main pre-trade check. Should complete in < 50ns.
    // Returns RejectReason::None if passed.
    RejectReason check_order(Price price, Quantity qty, Side side, Price reference_price = 0) const;

    // Instantly trip the kill switch from any thread
    static void trip_kill_switch();

    // Reset kill switch (administrative)
    static void reset_kill_switch();

    static bool is_kill_switch_active();

    void update_limits(const RiskLimits& new_limits) { limits_ = new_limits; }

private:
    RiskLimits limits_;
    
    // Global static lock-free kill switch
    static std::atomic<bool> global_kill_switch_;
};

} // namespace nexus
