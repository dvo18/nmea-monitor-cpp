#include "nmea/DataBuffer.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <vector>

using namespace nmea;

// ── Basic operations ───────────────────────────────────────────────────────

TEST(DataBuffer, StartsEmpty)
{
    DataBuffer<int, 8> buf;
    EXPECT_TRUE(buf.empty());
    EXPECT_EQ(buf.size(), 0u);
    EXPECT_EQ(buf.capacity(), 8u);
}

TEST(DataBuffer, PushIncrementsSize)
{
    DataBuffer<int, 8> buf;
    buf.push(42);
    EXPECT_EQ(buf.size(), 1u);
    EXPECT_FALSE(buf.empty());
}

TEST(DataBuffer, PopDecrementsSize)
{
    DataBuffer<int, 8> buf;
    buf.push(7);
    const int v = buf.pop();
    EXPECT_EQ(v, 7);
    EXPECT_TRUE(buf.empty());
}

TEST(DataBuffer, FifoOrder)
{
    DataBuffer<int, 8> buf;
    for (int i = 0; i < 5; ++i) buf.push(i);
    for (int i = 0; i < 5; ++i) EXPECT_EQ(buf.pop(), i);
}

TEST(DataBuffer, WrapAroundMaintainsFifoOrder)
{
    DataBuffer<int, 4> buf;
    for (int i = 0; i < 4; ++i) buf.push(i);
    for (int i = 0; i < 2; ++i) buf.pop();
    buf.push(10);
    buf.push(11);

    EXPECT_EQ(buf.pop(), 2);
    EXPECT_EQ(buf.pop(), 3);
    EXPECT_EQ(buf.pop(), 10);
    EXPECT_EQ(buf.pop(), 11);
}

// ── tryPush / tryPop ───────────────────────────────────────────────────────

TEST(DataBuffer, TryPushReturnsFalseWhenFull)
{
    DataBuffer<int, 2> buf;
    EXPECT_TRUE(buf.tryPush(1));
    EXPECT_TRUE(buf.tryPush(2));
    EXPECT_FALSE(buf.tryPush(3));
}

TEST(DataBuffer, TryPopReturnsNulloptWhenEmpty)
{
    DataBuffer<int, 4> buf;
    EXPECT_FALSE(buf.tryPop().has_value());
}

TEST(DataBuffer, TryPopReturnsValueWhenAvailable)
{
    DataBuffer<int, 4> buf;
    buf.push(99);
    auto v = buf.tryPop();
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, 99);
}

// ── Thread safety ──────────────────────────────────────────────────────────

TEST(DataBuffer, ConcurrentProducerConsumer)
{
    constexpr int kItems = 1000;
    DataBuffer<int, 64> buf;

    std::thread producer([&] {
        for (int i = 0; i < kItems; ++i) buf.push(i);
    });

    std::vector<int> received;
    received.reserve(kItems);

    std::thread consumer([&] {
        for (int i = 0; i < kItems; ++i) received.push_back(buf.pop());
    });

    producer.join();
    consumer.join();

    ASSERT_EQ(static_cast<int>(received.size()), kItems);
    for (int i = 0; i < kItems; ++i) EXPECT_EQ(received[i], i);
}