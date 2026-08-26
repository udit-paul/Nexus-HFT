#pragma once

#include <vector>
#include <cmath>
#include <cstdint>

namespace nexus {

// Computes running variance/volatility and moving average in O(1) time
// using Welford's online algorithm adapted for a sliding window.
class RollingStats {
public:
    explicit RollingStats(size_t window_size)
        : window_size_(window_size), values_(window_size, 0.0), index_(0), count_(0) {}

    void add(double value) {
        double old_val = values_[index_];
        values_[index_] = value;
        index_ = (index_ + 1) % window_size_;

        if (count_ < window_size_) {
            count_++;
            // Welford's online algorithm for adding
            double delta = value - mean_;
            mean_ += delta / count_;
            m2_ += delta * (value - mean_);
        } else {
            // Update removing old_val and adding value
            double mean_old = mean_;
            mean_ += (value - old_val) / window_size_;
            
            // Exact update for M2 in a sliding window
            m2_ += (value - mean_old) * (value - mean_) - (old_val - mean_old) * (old_val - mean_);
            if (m2_ < 0) m2_ = 0; // Prevent floating point drift below 0
        }
    }

    double mean() const {
        return mean_;
    }

    double variance() const {
        if (count_ < 2) return 0.0;
        return m2_ / (count_ - 1);
    }

    double std_dev() const {
        return std::sqrt(variance());
    }

    bool is_ready() const {
        return count_ == window_size_;
    }

private:
    size_t window_size_;
    std::vector<double> values_;
    size_t index_;
    size_t count_;

    double mean_{0.0};
    double m2_{0.0}; // Sum of squares of differences from the current mean
};

} // namespace nexus
