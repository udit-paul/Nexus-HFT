#include <gtest/gtest.h>
#include "nexus/concurrency/spsc_queue.hpp"
#include <thread>
#include <atomic>
#include <vector>

using namespace nexus;

TEST(SPSCQueueTest, BasicPushPop) {
    SPSCQueue<int> queue(8); // Power of 2 capacity
    
    int val = 0;
    EXPECT_FALSE(queue.pop(val)); // Empty initially
    
    EXPECT_TRUE(queue.push(42));
    EXPECT_TRUE(queue.pop(val));
    EXPECT_EQ(val, 42);
    
    EXPECT_FALSE(queue.pop(val)); // Empty again
}

TEST(SPSCQueueTest, QueueFull) {
    SPSCQueue<int> queue(4); // 4 capacity means it can hold 3 items due to ring buffer mechanics
    
    EXPECT_TRUE(queue.push(1));
    EXPECT_TRUE(queue.push(2));
    EXPECT_TRUE(queue.push(3));
    EXPECT_FALSE(queue.push(4)); // Should be full
    
    int val = 0;
    EXPECT_TRUE(queue.pop(val));
    EXPECT_EQ(val, 1);
    
    EXPECT_TRUE(queue.push(4)); // Should have space now
    EXPECT_FALSE(queue.push(5)); // Full again
}

TEST(SPSCQueueTest, ConcurrentProducerConsumer) {
    constexpr int NUM_MESSAGES = 100000;
    SPSCQueue<int> queue(1024);
    
    std::atomic<bool> producer_done{false};
    std::atomic<int> received_count{0};
    
    std::thread producer([&]() {
        for (int i = 0; i < NUM_MESSAGES; ++i) {
            // Spin until push succeeds
            while (!queue.push(i)) {
                std::this_thread::yield();
            }
        }
        producer_done.store(true, std::memory_order_release);
    });
    
    std::thread consumer([&]() {
        int expected = 0;
        int val = 0;
        
        while (true) {
            if (queue.pop(val)) {
                EXPECT_EQ(val, expected);
                expected++;
                received_count++;
            } else if (producer_done.load(std::memory_order_acquire)) {
                // Check once more in case something was pushed right before done flag was set
                while (queue.pop(val)) {
                    EXPECT_EQ(val, expected);
                    expected++;
                    received_count++;
                }
                break;
            }
        }
    });
    
    producer.join();
    consumer.join();
    
    EXPECT_EQ(received_count.load(), NUM_MESSAGES);
}
