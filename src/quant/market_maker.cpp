#include "nexus/quant/market_maker.hpp"

namespace nexus {

double AvellanedaStoikov::calculate_reservation_price(double mid_price, int64_t inventory, double t) const {
    // R(s, q, t) = s - q * gamma * sigma^2 * (T - t)
    double time_left = params_.T - t;
    if (time_left < 0) time_left = 0;
    
    double variance = params_.sigma * params_.sigma;
    double inventory_risk_penalty = static_cast<double>(inventory) * params_.gamma * variance * time_left;
    
    return mid_price - inventory_risk_penalty;
}

double AvellanedaStoikov::calculate_optimal_spread(double t) const {
    double time_left = params_.T - t;
    if (time_left < 0) time_left = 0;
    
    double variance = params_.sigma * params_.sigma;
    
    // Spread = gamma * sigma^2 * (T-t) + (2/gamma) * ln(1 + gamma/k)
    double time_component = params_.gamma * variance * time_left;
    double density_component = (2.0 / params_.gamma) * std::log(1.0 + (params_.gamma / params_.k));
    
    return time_component + density_component;
}

Quotes AvellanedaStoikov::get_quotes(double mid_price, int64_t inventory, double t) const {
    double reservation_price = calculate_reservation_price(mid_price, inventory, t);
    double spread = calculate_optimal_spread(t);
    
    // Asymmetric quotes around the reservation price
    double raw_bid = reservation_price - (spread / 2.0);
    double raw_ask = reservation_price + (spread / 2.0);
    
    // Convert back to fixed-point integer pricing
    // Note: In real life you would round to the nearest tick size
    return Quotes{
        static_cast<Price>(std::round(raw_bid)),
        static_cast<Price>(std::round(raw_ask))
    };
}

} // namespace nexus
