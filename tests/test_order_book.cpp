#include <gtest/gtest.h>
#include "nexus/core/order_book.hpp"
#include "nexus/core/memory_pool.hpp"

using namespace nexus;

class OrderBookTest : public ::testing::Test {
protected:
    void SetUp() override {
        pool = std::make_unique<MemoryPool<OrderNode>>(1000);
    }
    
    std::unique_ptr<MemoryPool<OrderNode>> pool;
    OrderBook book;
};

TEST_F(OrderBookTest, InsertAndBestBidAsk) {
    OrderNode* buy1 = pool->allocate(1, 100, 10, Side::Buy);
    OrderNode* buy2 = pool->allocate(2, 101, 20, Side::Buy);
    OrderNode* sell1 = pool->allocate(3, 102, 15, Side::Sell);

    book.insert_order(buy1);
    book.insert_order(buy2);
    book.insert_order(sell1);

    PriceLevel* best_bid = book.get_best_bid();
    ASSERT_NE(best_bid, nullptr);
    EXPECT_EQ(best_bid->price, 101);
    EXPECT_EQ(best_bid->total_volume, 20);

    PriceLevel* best_ask = book.get_best_ask();
    ASSERT_NE(best_ask, nullptr);
    EXPECT_EQ(best_ask->price, 102);
    EXPECT_EQ(best_ask->total_volume, 15);
}

TEST_F(OrderBookTest, CancelOrder) {
    OrderNode* buy = pool->allocate(1, 100, 10, Side::Buy);
    book.insert_order(buy);
    
    EXPECT_NE(book.get_best_bid(), nullptr);
    
    book.cancel_order(buy);
    
    EXPECT_EQ(book.get_best_bid(), nullptr);
}

TEST_F(OrderBookTest, ReduceOrder) {
    OrderNode* buy = pool->allocate(1, 100, 10, Side::Buy);
    book.insert_order(buy);
    
    book.reduce_order(buy, 4);
    
    PriceLevel* best_bid = book.get_best_bid();
    ASSERT_NE(best_bid, nullptr);
    EXPECT_EQ(best_bid->total_volume, 4);
    EXPECT_EQ(buy->qty, 4);
}
