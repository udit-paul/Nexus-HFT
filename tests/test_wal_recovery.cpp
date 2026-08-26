#include <gtest/gtest.h>
#include "nexus/persistence/mmap_wal.hpp"
#include <fstream>
#include <cstdio>
#include <cstring>

using namespace nexus;

class WALTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_file = "test_wal.dat";
        std::remove(test_file.c_str());
    }

    void TearDown() override {
        std::remove(test_file.c_str());
    }

    std::string test_file;
};

TEST_F(WALTest, OpenAndClose) {
    MmapWAL wal(test_file, 1024 * 1024); // 1MB
    EXPECT_TRUE(wal.open());
    wal.close();
    
    // File should exist and be exactly 1MB
    std::ifstream file(test_file, std::ios::binary | std::ios::ate);
    EXPECT_TRUE(file.is_open());
    EXPECT_EQ(file.tellg(), 1024 * 1024);
}

TEST_F(WALTest, AppendAndRecover) {
    // 1. Write data to WAL
    {
        MmapWAL wal(test_file, 1024 * 1024);
        EXPECT_TRUE(wal.open());
        
        NewOrderMsg msg1;
        msg1.header.type = MsgType::NewOrder;
        msg1.header.length = sizeof(NewOrderMsg);
        msg1.client_order_id = 100;
        msg1.price = 50;
        msg1.qty = 10;
        msg1.side = Side::Buy;
        
        EXPECT_TRUE(wal.append_order(msg1));
        
        NewOrderMsg msg2;
        msg2.header.type = MsgType::NewOrder;
        msg2.header.length = sizeof(NewOrderMsg);
        msg2.client_order_id = 101;
        msg2.price = 55;
        msg2.qty = 20;
        msg2.side = Side::Sell;
        
        EXPECT_TRUE(wal.append_order(msg2));
        
        wal.close();
    }
    
    // 2. Open existing WAL and recover data
    {
        MmapWAL recovery_wal(test_file, 1024 * 1024);
        EXPECT_TRUE(recovery_wal.open());
        
        void* data = recovery_wal.data();
        ASSERT_NE(data, nullptr);
        
        // Recover msg1
        NewOrderMsg recovered1;
        std::memcpy(&recovered1, data, sizeof(NewOrderMsg));
        EXPECT_EQ(recovered1.client_order_id, 100);
        EXPECT_EQ(recovered1.price, 50);
        EXPECT_EQ(recovered1.side, Side::Buy);
        
        // Recover msg2
        NewOrderMsg recovered2;
        std::memcpy(&recovered2, static_cast<char*>(data) + sizeof(NewOrderMsg), sizeof(NewOrderMsg));
        EXPECT_EQ(recovered2.client_order_id, 101);
        EXPECT_EQ(recovered2.price, 55);
        EXPECT_EQ(recovered2.side, Side::Sell);
        
        recovery_wal.close();
    }
}
