#include <string>
#include <gtest/gtest.h>

#include "util/tokenizer.h"

TEST(TokenizerTest, SimpleTest)
{
    std::string str("test1 test2");
    util::Tokenizer tok;
    tok.split(str);
    ASSERT_EQ(2, tok.size());
    EXPECT_EQ("test1", tok[0]);
    EXPECT_EQ("test2", tok[1]);
}

TEST(TokenizerTest, DoubleDelim)
{
    std::string str(":test1::test2:test3::");
    util::Tokenizer tok;

    tok.split(str, ':', true);
    ASSERT_EQ(7, tok.size());
    EXPECT_EQ("", tok[0]);
    EXPECT_EQ("test1", tok[1]);
    EXPECT_EQ("", tok[2]);
    EXPECT_EQ("test2", tok[3]);
    EXPECT_EQ("test3", tok[4]);
    EXPECT_EQ("", tok[5]);
    EXPECT_EQ("", tok[6]);

    tok.split(str, ':', false);
    ASSERT_EQ(3, tok.size());
    EXPECT_EQ("test1", tok[0]);
    EXPECT_EQ("test2", tok[1]);
    EXPECT_EQ("test3", tok[2]);
}
