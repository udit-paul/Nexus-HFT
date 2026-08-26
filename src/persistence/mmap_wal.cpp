#include "nexus/persistence/mmap_wal.hpp"
#include <stdexcept>
#include <cstring>
#include <iostream>

#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace nexus {

MmapWAL::MmapWAL(const std::string& filepath, std::size_t initial_size)
    : filepath_(filepath), file_size_(initial_size) {}

MmapWAL::~MmapWAL() {
    close();
}

bool MmapWAL::open() {
#if defined(_WIN32)
    file_handle_ = CreateFileA(filepath_.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                               OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file_handle_ == INVALID_HANDLE_VALUE) return false;

    // Resize file
    LARGE_INTEGER size;
    size.QuadPart = file_size_;
    SetFilePointerEx(file_handle_, size, nullptr, FILE_BEGIN);
    SetEndOfFile(file_handle_);

    mapping_handle_ = CreateFileMappingA(file_handle_, nullptr, PAGE_READWRITE, 0, 0, nullptr);
    if (!mapping_handle_) return false;

    mapped_data_ = MapViewOfFile(mapping_handle_, FILE_MAP_ALL_ACCESS, 0, 0, file_size_);
    if (!mapped_data_) return false;
#else
    fd_ = ::open(filepath_.c_str(), O_RDWR | O_CREAT, 0666);
    if (fd_ == -1) return false;

    if (ftruncate(fd_, file_size_) == -1) return false;

    mapped_data_ = mmap(nullptr, file_size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
    if (mapped_data_ == MAP_FAILED) {
        mapped_data_ = nullptr;
        return false;
    }
#endif
    return true;
}

void MmapWAL::close() {
    if (mapped_data_) {
#if defined(_WIN32)
        UnmapViewOfFile(mapped_data_);
        CloseHandle(mapping_handle_);
        CloseHandle(file_handle_);
#else
        munmap(mapped_data_, file_size_);
        ::close(fd_);
#endif
        mapped_data_ = nullptr;
    }
}

bool MmapWAL::append_order(const NewOrderMsg& msg) {
    if (!mapped_data_ || write_offset_ + sizeof(NewOrderMsg) > file_size_) {
        // In a real system, you would unmap, double the file size, and remap here.
        return false;
    }

    // Direct memcpy into memory mapped region (OS will flush to disk in background)
    std::memcpy(static_cast<char*>(mapped_data_) + write_offset_, &msg, sizeof(NewOrderMsg));
    write_offset_ += sizeof(NewOrderMsg);
    
    return true;
}

} // namespace nexus
