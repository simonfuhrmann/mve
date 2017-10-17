// Test cases for the string utils.
// Written by Simon Fuhrmann.

#include <gtest/gtest.h>

#include "util/strings.h"

TEST(StringTest, LowerUpperCaseTest)
{
    /* Test to lowerstring and to upper string. */
    std::string str = "Test aAzZ 012349 STRING !! #$%";
    std::string str_uc = "TEST AAZZ 012349 STRING !! #$%";
    std::string str_lc = "test aazz 012349 string !! #$%";
    EXPECT_EQ(str_uc, util::string::uppercase(str));
    EXPECT_EQ(str_lc, util::string::lowercase(str));
}

TEST(StringTest, StringConversionTest)
{
    EXPECT_EQ("1230", util::string::get(1230));
    EXPECT_EQ("0.1", util::string::get(0.1));
    EXPECT_EQ("0.1", util::string::get(0.1f));
    EXPECT_EQ("0.333", util::string::get(0.333));
    EXPECT_EQ("0.333333", util::string::get(0.333333333333333));
    EXPECT_EQ("10.1235", util::string::get(10.12345678));  // ??

    EXPECT_EQ("123", util::string::get_fixed(123, 4));
    EXPECT_EQ("1.2340", util::string::get_fixed(1.234, 4));
    EXPECT_EQ("1.0000", util::string::get_fixed(1.0f, 4));
    EXPECT_EQ("1.0000", util::string::get_fixed(1.00001, 4));
    EXPECT_EQ("1.0001", util::string::get_fixed(1.00009, 4));

    EXPECT_EQ("0001", util::string::get_filled(1, 4, '0'));
    EXPECT_EQ("   1", util::string::get_filled(1, 4, ' '));
    EXPECT_EQ("--10", util::string::get_filled(10, 4, '-'));
    EXPECT_EQ("12345", util::string::get_filled(12345, 4, '-'));
    EXPECT_EQ("10.1235", util::string::get_filled(10.12349, 4, '-'));  // ??

    EXPECT_EQ(10.1234, util::string::convert<double>("10.1234"));
    EXPECT_EQ(10.1234f, util::string::convert<float>("10.1234"));

    // Strict conversion will throw if not all chars are consumed.
    ASSERT_THROW(util::string::convert<double>("10.1234asfd"), std::invalid_argument);
    ASSERT_THROW(util::string::convert<int>("10.1234asfd"), std::invalid_argument);
    ASSERT_THROW(util::string::convert<char>("10.1234asfd"), std::invalid_argument);
    EXPECT_EQ("1.23asfd", util::string::convert<std::string>("1.23asfd"));

    EXPECT_EQ(10.1234, util::string::convert<double>("10.1234asfd", false));
    EXPECT_EQ(10, util::string::convert<int>("10.1234asfd", false));
    EXPECT_EQ('1', util::string::convert<char>("10.1234asfd", false));

    // Strict conversion will always throw on empty argument.
    ASSERT_THROW(util::string::convert<float>(""), std::invalid_argument);
    ASSERT_THROW(util::string::convert<int>(""), std::invalid_argument);
    ASSERT_THROW(util::string::convert<char>(""), std::invalid_argument);
    ASSERT_THROW(util::string::convert<std::string>(""), std::invalid_argument);

    EXPECT_EQ(0.0f, util::string::convert<float>("", false));
    EXPECT_EQ(0, util::string::convert<int>("", false));
    EXPECT_EQ('\0', util::string::convert<char>("", false));
    EXPECT_EQ("", util::string::convert<std::string>("", false));
}

TEST(StringTest, LeftRightSubstringTest)
{
    std::string str = "123456";
    EXPECT_EQ("1234", util::string::left(str, 4));
    EXPECT_EQ("3456", util::string::right(str, 4));
    EXPECT_EQ("123456", util::string::left(str, 8));
    EXPECT_EQ("123456", util::string::right(str, 8));
}

TEST(StringTest, ClipAndChopTest)
{
    std::string str1 = "\t  \t test\t ";
    std::string str2 = " \t  \t test\t";
    EXPECT_EQ("test", util::string::clipped_whitespaces(str1));
    EXPECT_EQ("test", util::string::clipped_whitespaces(str2));

    std::string str3 = "test\n";
    std::string str4 = "test\r\n";
    std::string str5 = "test\n\r";
    std::string str6 = "test\n\t";
    std::string str7 = "test\n\n\n";
    EXPECT_EQ("test", util::string::clipped_newlines(str3));
    EXPECT_EQ("test", util::string::clipped_newlines(str4));
    EXPECT_EQ("test", util::string::clipped_newlines(str5));
    EXPECT_EQ(str6, util::string::clipped_newlines(str6));
    EXPECT_EQ("test", util::string::clipped_newlines(str7));
}

TEST(StringTest, PunctateTest)
{
    std::string str = "1234567890";
    EXPECT_EQ("12.3456.7890", util::string::punctated(str, '.', 4));
    EXPECT_EQ("1.234.567.890", util::string::punctated(str, '.', 3));
    EXPECT_EQ("12.34.56.78.90", util::string::punctated(str, '.', 2));
    EXPECT_EQ("1.2.3.4.5.6.7.8.9.0", util::string::punctated(str, '.', 1));
    EXPECT_EQ("1234567890", util::string::punctated(str, '.', 0));
    EXPECT_EQ("", util::string::punctated("", '.', 2));
    EXPECT_EQ("", util::string::punctated("", '.', 1));
    EXPECT_EQ("", util::string::punctated("", '.', 0));
    EXPECT_EQ("1", util::string::punctated("1", '.', 3));
    EXPECT_EQ("12", util::string::punctated("12", '.', 3));
    EXPECT_EQ("123", util::string::punctated("123", '.', 3));
    EXPECT_EQ("1.234", util::string::punctated("1234", '.', 3));
}

TEST(StringTest, WordWrapTest)
{
    std::string str1 = "some longword";
    EXPECT_EQ("some\nlongword", util::string::wordwrap(str1.c_str(), 4));
    EXPECT_EQ("some\nlongword", util::string::wordwrap(str1.c_str(), 12));
    EXPECT_EQ("some longword", util::string::wordwrap(str1.c_str(), 13));

    std::string str2 = "some words  ";
    EXPECT_EQ("some\nwords\n ", util::string::wordwrap(str2.c_str(), 5));

    std::string str3 = "some\nlong word";
    EXPECT_EQ("some\nlong word", util::string::wordwrap(str3.c_str(), 9));
}

TEST(StringTest, StringNormalizationTest)
{
    std::string str = "  string \t that\tis  pretty messy  \t";
    std::string str_gt = " string that is pretty messy ";
    util::string::normalized(str);
    EXPECT_EQ(str_gt, util::string::normalized(str));
}
