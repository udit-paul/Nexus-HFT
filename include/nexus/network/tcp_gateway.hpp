#pragma once

#include "nexus/network/binary_protocol.hpp"
#include "nexus/concurrency/spsc_queue.hpp"
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <atomic>

// Forward declarations for Boost.Asio to avoid requiring boost headers for clients
namespace boost { namespace asio { class io_context; } }
namespace boost { namespace asio { namespace ip { class tcp; } } }

namespace nexus {

class TcpSession;

class TcpGateway {
public:
    // Takes a reference to the inbound SPSC queue that feeds the Matching Engine
    TcpGateway(uint16_t port, SPSCQueue<NewOrderMsg>& inbound_queue);
    ~TcpGateway();

    void start();
    void stop();

private:
    void accept_connections();

    uint16_t port_;
    SPSCQueue<NewOrderMsg>& inbound_queue_;
    
    // std::unique_ptr<boost::asio::io_context> io_context_;
    // std::unique_ptr<boost::asio::ip::tcp> acceptor_;
    std::vector<std::shared_ptr<TcpSession>> sessions_;
    
    std::thread io_thread_;
    std::atomic<bool> running_{false};
};

} // namespace nexus
