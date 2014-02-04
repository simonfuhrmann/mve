// Test cases for endian conversions.
// Written by Simon Fuhrmann.

#include <gtest/gtest.h>

#include "util/endian.h"

TEST(EndianTest, TestByteSwap)
{
    short val1 = 0x1234;
    util::system::byte_swap<2>(reinterpret_cast<char*>(&val1));
    EXPECT_EQ((short)0x3412, val1);

    unsigned short val2 = 0x1234;
    util::system::byte_swap<2>(reinterpret_cast<char*>(&val2));
    EXPECT_EQ((unsigned short)0x3412, val2);

    int val3 = 0x12345678;
    util::system::byte_swap<4>(reinterpret_cast<char*>(&val3));
    EXPECT_EQ(static_cast<int>(0x78563412), val3);

    unsigned int val4 = 0x12345678;
    util::system::byte_swap<4>(reinterpret_cast<char*>(&val4));
    EXPECT_EQ(static_cast<unsigned int>(0x78563412), val4);

    long long int val5 = 0x0123456789abcdef;
    util::system::byte_swap<8>(reinterpret_cast<char*>(&val5));
    EXPECT_EQ(static_cast<long long int>(0xefcdab8967452301), val5);

    unsigned long long int val6 = 0x0123456789abcdef;
    util::system::byte_swap<8>(reinterpret_cast<char*>(&val6));
    EXPECT_EQ(static_cast<unsigned long long int>(0xefcdab8967452301), val6);
}

TEST(EndianTest, DoubleSwaps)
{
    short short_val = 0x1234;
    int int_val = 0x12345678;
    long long int long_val = 0x0123456789abcdef;

    using util::system::letoh;
    using util::system::betoh;
    EXPECT_EQ(short_val, letoh(letoh(short_val)));
    EXPECT_EQ(short_val, betoh(betoh(short_val)));
    EXPECT_EQ(int_val, letoh(letoh(int_val)));
    EXPECT_EQ(int_val, betoh(betoh(int_val)));
    EXPECT_EQ(long_val, letoh(letoh(long_val)));
    EXPECT_EQ(long_val, betoh(betoh(long_val)));
}
