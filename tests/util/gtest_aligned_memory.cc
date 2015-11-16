#include <gtest/gtest.h>

#include "util/aligned_memory.h"

TEST(AlignedMemoryTest, AlignmentTest)
{
    {
        util::AlignedMemory<float, 16> mem;
        mem.resize(1);
        uintptr_t ptr = reinterpret_cast<uintptr_t>(mem.data());
        EXPECT_EQ(0, ptr & 15);
    }
    {
        util::AlignedMemory<float, 32> mem;
        mem.resize(1);
        uintptr_t ptr = reinterpret_cast<uintptr_t>(mem.data());
        EXPECT_EQ(0, ptr & 31);
    }
    {
        util::AlignedMemory<float, 64> mem;
        mem.resize(1);
        uintptr_t ptr = reinterpret_cast<uintptr_t>(mem.data());
        EXPECT_EQ(0, ptr & 63);
    }
    {
        util::AlignedMemory<float, 128> mem;
        mem.resize(1);
        uintptr_t ptr = reinterpret_cast<uintptr_t>(mem.data());
        EXPECT_EQ(0, ptr & 127);
    }
    {
        util::AlignedMemory<float, 256> mem;
        mem.resize(1);
        uintptr_t ptr = reinterpret_cast<uintptr_t>(mem.data());
        EXPECT_EQ(0, ptr & 255);
    }
    {
        util::AlignedMemory<float, 512> mem;
        mem.resize(1);
        uintptr_t ptr = reinterpret_cast<uintptr_t>(mem.data());
        EXPECT_EQ(0, ptr & 511);
    }
}

TEST(AlignedMemoryTest, IterationTest)
{
    util::AlignedMemory<float, 16> mem;
    mem.resize(12);
    int num = 0;
    for (util::AlignedMemory<float, 16>::iterator i = mem.begin(); i != mem.end(); ++i)
        num++;
    EXPECT_EQ(12, num);
}

TEST(AlignedMemoryTest, IterationConstTest)
{
    util::AlignedMemory<float, 16> const mem(12);
    int num = 0;
    for (util::AlignedMemory<float, 16>::const_iterator i = mem.begin(); i != mem.end(); ++i)
        num++;
    EXPECT_EQ(12, num);
}

TEST(AlignedMemoryTest, IterationNULLTest)
{
    util::AlignedMemory<float, 16> mem;
    int num = 0;
    for (util::AlignedMemory<float, 16>::iterator i = mem.begin(); i != mem.end(); ++i)
        num++;
    EXPECT_EQ(0, num);
}

TEST(AlignedMemoryTest, AccessTest)
{
    util::AlignedMemory<float, 16> mem(3);
    int num = 0;
    for (util::AlignedMemory<float, 16>::iterator i = mem.begin(); i != mem.end(); ++i)
        *i = static_cast<float>(num++);
    EXPECT_EQ(0.0f, mem[0]);
    EXPECT_EQ(1.0f, mem[1]);
    EXPECT_EQ(2.0f, mem[2]);
}

TEST(AlignedMemoryTest, CopyAndAssignTest)
{
    util::AlignedMemory<float, 16> mem(10);
    for (int i = 0; i < 10; ++i)
        mem[i] = static_cast<float>(i);

    // Make sure it's not the same memory but the same content.
    // Test copy constructor.
    util::AlignedMemory<float, 16> mem2(mem);
    EXPECT_NE(mem.begin(), mem2.begin());
    for (int i = 0; i < 10; ++i)
        EXPECT_EQ(mem[i], mem2[i]);

    // Test assignment operator.
    util::AlignedMemory<float, 16> mem3;
    mem3 = mem;
    EXPECT_NE(mem.begin(), mem3.begin());
    for (int i = 0; i < 10; ++i)
        EXPECT_EQ(mem[i], mem3[i]);
}
