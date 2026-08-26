#pragma once

#include "nexus/network/binary_protocol.hpp"
#include <string>
#include <cstdint>

namespace nexus {

class MmapWAL {
public:
    explicit MmapWAL(const std::string& filepath, std::size_t initial_size = 1024 * 1024 * 100); // 100MB default
    ~MmapWAL();

    bool open();
    void close();

    // Fast append of binary struct
    bool append_order(const NewOrderMsg& msg);

    // Read interface for recovery
    void* data() const { return mapped_data_; }
    std::size_t size() const { return file_size_; }
    std::size_t written_bytes() const { return write_offset_; }

private:
    std::string filepath_;
    std::size_t file_size_;
    std::size_t write_offset_{0};
    
    void* mapped_data_{nullptr};
    
#if defined(_WIN32)
    void* file_handle_{nullptr};
    void* mapping_handle_{nullptr};
#else
    int fd_{-1};
#endif
};

} // namespace nexus
