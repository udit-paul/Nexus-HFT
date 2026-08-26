#include <iostream>
#include <string>
#include <sstream>
#include "nexus/engine/matching_engine.hpp"
#include "nexus/core/memory_pool.hpp"
#include "nexus/core/rdtsc.hpp"

using namespace nexus;

// One-time calibration — called once at startup, not on the hot path.
static RdtscCalibration g_tsc_cal;

void print_book(const OrderBook& book) {
    std::cout << "BOOK {\"bids\":[";
    bool first = true;
    int count = 0;
    for (const auto& [price, level] : book.get_bids()) {
        if (!first) std::cout << ",";
        std::cout << "[" << (price / 100.0) << "," << level.total_volume << "]";
        first = false;
        if (++count >= 10) break; // limit to top 10
    }
    std::cout << "],\"asks\":[";
    first = true;
    count = 0;
    for (const auto& [price, level] : book.get_asks()) {
        if (!first) std::cout << ",";
        std::cout << "[" << (price / 100.0) << "," << level.total_volume << "]";
        first = false;
        if (++count >= 10) break; // limit to top 10
    }
    std::cout << "]}" << std::endl;
}

int main() {
    // Calibrate RDTSC once at startup (spins for ~10ms)
    g_tsc_cal.calibrate();

    auto engine = std::make_unique<MatchingEngine>();
    auto pool = std::make_unique<MemoryPool<OrderNode>>(1000000);
    OrderId next_id = 1;
    
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        
        std::istringstream iss(line);
        std::string command;
        iss >> command;
        
        if (command == "CLEAR") {
            engine = std::make_unique<MatchingEngine>();
            pool = std::make_unique<MemoryPool<OrderNode>>(1000000);
            next_id = 1;
            print_book(engine->get_book());
            std::cout << "CLEAR_ACK" << std::endl;
        } else if (command == "BUY" || command == "SELL") {
            double price_input;
            Quantity qty;
            if (iss >> price_input >> qty) {
                Price price = static_cast<Price>(price_input * 100.0 + 0.5);
                Side side = (command == "BUY") ? Side::Buy : Side::Sell;
                
                OrderNode* order = pool->allocate();
                if (!order) {
                    std::cerr << "ERR Memory pool exhausted" << std::endl;
                    continue;
                }
                
                // Initialize the allocated node
                order->id    = next_id++;
                order->price = price;
                order->qty   = qty;
                order->side  = side;
                order->prev  = nullptr;
                order->next  = nullptr;
                
                // ── Hot-path latency measurement via RDTSC (not chrono syscall) ──
                uint64_t t_start = rdtscp();
                auto trades = engine->process_limit_order(order);
                uint64_t t_end   = rdtscp();

                // Convert cycles → nanoseconds using the pre-calibrated ratio
                uint64_t latency_ns = g_tsc_cal.to_ns_u64(t_end - t_start);
                
                for (const auto& trade : trades) {
                    std::cout << "TRADE " << (trade.price / 100.0) << " " << trade.qty << std::endl;
                }
                
                print_book(engine->get_book());
                std::cout << "LATENCY " << latency_ns << std::endl;
            } else {
                std::cerr << "ERR Invalid format" << std::endl;
            }
        } else {
            std::cerr << "ERR Unknown command: " << command << std::endl;
        }
    }
    
    return 0;
}
