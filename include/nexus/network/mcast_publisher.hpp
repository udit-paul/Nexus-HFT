#pragma once

#include "nexus/network/binary_protocol.hpp"
#include <string>
#include <vector>

namespace nexus {

class McastPublisher {
public:
    McastPublisher(const std::string& multicast_ip, uint16_t port);
    ~McastPublisher();

    void start();
    void stop();

    // Broadcast Top-Of-Book or Aggregate Depth (L2)
    void publish_l2_snapshot(Price best_bid, Quantity bid_vol, Price best_ask, Quantity ask_vol);

    // Broadcast individual order events (L3)
    void publish_l3_event(const ExecutionReportMsg& report);

private:
    std::string multicast_ip_;
    uint16_t port_;
    
    // In reality, this would hold Boost.Asio UDP sockets
    // boost::asio::io_context io_context_;
    // boost::asio::ip::udp::endpoint endpoint_;
    // boost::asio::ip::udp::socket socket_;
};

} // namespace nexus
