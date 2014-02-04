#include <gtest/gtest.h>

#include "util/aligned_memory.h"

TEST(AlignedMemoryTest, AlignmentTest)
{
    {
        util::AlignedMemory<float, 16> mem;
        mem.allocate(1);
        uintptr_t ptr = reinterpret_cast<uintptr_t>(mem.begin());
        EXPECT_EQ(0, ptr & 15);
    }
    {
        util::AlignedMemory<float, 32> mem;
        mem.allocate(1);
        uintptr_t ptr = reinterpret_cast<uintptr_t>(mem.begin());
        EXPECT_EQ(0, ptr & 31);
    }
    {
        util::AlignedMemory<float, 64> mem;
        mem.allocate(1);
        uintptr_t ptr = reinterpret_cast<uintptr_t>(mem.begin());
        EXPECT_EQ(0, ptr & 63);
    }
    {
        util::AlignedMemory<float, 128> mem;
        mem.allocate(1);
        uintptr_t ptr = reinterpret_cast<uintptr_t>(mem.begin());
        EXPECT_EQ(0, ptr & 127);
    }
    {
        util::AlignedMemory<float, 256> mem;
        mem.allocate(1);
        uintptr_t ptr = reinterpret_cast<uintptr_t>(mem.begin());
        EXPECT_EQ(0, ptr & 255);
    }
    {
        util::AlignedMemory<float, 512> mem;
        mem.allocate(1);
        uintptr_t ptr = reinterpret_cast<uintptr_t>(mem.begin());
        EXPECT_EQ(0, ptr & 511);
    }
}

TEST(AlignedMemoryTest, IterationTest)
{
    util::AlignedMemory<float, 16> mem;
    mem.allocate(12);
    int num = 0;
    for (float* i = mem.begin(); i != mem.end(); ++i)
        num++;
    EXPECT_EQ(12, num);
}

TEST(AlignedMemoryTest, IterationConstTest)
{
    util::AlignedMemory<float, 16> const mem(12);
    int num = 0;
    for (float const* i = mem.begin(); i != mem.end(); ++i)
        num++;
    EXPECT_EQ(12, num);
}

TEST(AlignedMemoryTest, IterationNULLTest)
{
    util::AlignedMemory<float, 16> mem;
    int num = 0;
    for (float* i = mem.begin(); i != mem.end(); ++i)
        num++;
    EXPECT_EQ(0, num);
}

TEST(AlignedMemoryTest, AccessTest)
{
    util::AlignedMemory<float, 16> mem(3);
    int num = 0;
    for (float* i = mem.begin(); i != mem.end(); ++i)
        *i = static_cast<float>(num++);
    EXPECT_EQ(0.0f, mem[0]);
    EXPECT_EQ(1.0f, mem[1]);
    EXPECT_EQ(2.0f, mem[2]);
}
