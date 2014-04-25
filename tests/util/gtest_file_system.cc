// Test cases for the file system interface.
// Written by Simon Fuhrmann.

#include <gtest/gtest.h>

#include "util/file_system.h"

TEST(FileSystemTest, BasenameTest)
{
    EXPECT_EQ("", util::fs::basename("/"));
    EXPECT_EQ("asdf", util::fs::basename("/asdf"));
    EXPECT_EQ("asdf", util::fs::basename("/asdf/"));
    EXPECT_EQ("asdf", util::fs::basename("/asdf/////"));
    EXPECT_EQ("ghjk", util::fs::basename("/asdf/ghjk"));

    EXPECT_EQ("", util::fs::basename(""));
    EXPECT_EQ("asdf", util::fs::basename("asdf"));
    EXPECT_EQ("asdf", util::fs::basename("asdf/"));
    EXPECT_EQ("asdf", util::fs::basename("asdf/////"));
    EXPECT_EQ("ghjk", util::fs::basename("asdf/ghjk"));
}

TEST(FileSystemTest, DirnameTest)
{
    EXPECT_EQ("/", util::fs::dirname("/"));
    EXPECT_EQ("/", util::fs::dirname("/asdf"));
    EXPECT_EQ("/", util::fs::dirname("/asdf/"));
    EXPECT_EQ("/", util::fs::dirname("/asdf/////"));
    EXPECT_EQ("/asdf", util::fs::dirname("/asdf/ghjk"));

    EXPECT_EQ(".", util::fs::dirname(""));
    EXPECT_EQ(".", util::fs::dirname("asdf"));
    EXPECT_EQ(".", util::fs::dirname("asdf/"));
    EXPECT_EQ(".", util::fs::dirname("asdf/////"));
    EXPECT_EQ("asdf", util::fs::dirname("asdf/ghjk"));

#ifndef _WIN32
    // The following tests don't work on OSX and Windows.
    //util::fs::set_cwd("/tmp");
    //EXPECT_EQ("/tmp", util::fs::dirname("test.txt"));
    //EXPECT_EQ("/tmp", util::fs::dirname(""));
    //EXPECT_EQ("/tmp/blah", util::fs::dirname("blah/test.txt"));
#endif
}

TEST(FileSystemTest, AbspathTest)
{
    EXPECT_EQ(util::fs::get_cwd_string() + "/.", util::fs::abspath("."));
    EXPECT_EQ(util::fs::get_cwd_string() + "/../dir", util::fs::abspath(util::fs::dirname("../dir/file")));
}

TEST(FileSystemTest, IsAbsoluteTest)
{
#ifdef _WIN32
    EXPECT_TRUE(util::fs::is_absolute("C:/Windows"));
#else
    EXPECT_TRUE(util::fs::is_absolute("/boot"));
#endif

    EXPECT_FALSE(util::fs::is_absolute("../debug.log"));
}

TEST(FileSystemTest, SanitizePathTest)
{
    EXPECT_EQ("", util::fs::sanitize_path(""));
    EXPECT_EQ("/", util::fs::sanitize_path("/////"));
    EXPECT_EQ("C:/Windows/System32/drivers/etc/hosts.txt", util::fs::sanitize_path("C:\\Windows\\System32\\drivers\\/etc/hosts.txt"));
    EXPECT_EQ("/usr/local/../../var/tmp", util::fs::sanitize_path("/usr/local/../..//var/tmp/"));
}

TEST(FileSystemTest, JoinPathTest)
{
#ifdef _WIN32
    EXPECT_EQ("C:/Windows/System32/drivers/etc/hosts.txt", util::fs::join_path("C:\\Windows\\System32\\drivers\\", "/etc/hosts.txt"));
#else
    EXPECT_EQ("/usr/local/share/ca-certificates", util::fs::join_path("/usr/local", "share/ca-certificates"));
    EXPECT_EQ("/var/spool/mail", util::fs::join_path("/usr/local", "/var/spool/mail"));
#endif
}

TEST(FileSystemTest, ReplaceExtensionTest)
{
    EXPECT_EQ("file.bbb", util::fs::replace_extension("file.aaa", "bbb"));
    EXPECT_EQ("file.bbb", util::fs::replace_extension("file", "bbb"));
    EXPECT_EQ("/a/file.b", util::fs::replace_extension("/a/file.a", "b"));
    EXPECT_EQ("/a.b/c.e", util::fs::replace_extension("/a.b/c.d", "e"));
    EXPECT_EQ("/a.b/cd.e", util::fs::replace_extension("/a.b/cd", "e"));
}

TEST(FileSystemTest, TestFileIO)
{
#if defined(_WIN32)
    std::string tempfile("C:/temp/tempfile.txt");
#else
    std::string tempfile("/tmp/tempfile.txt");
#endif

    std::string data = "This is\na test string\n";
    util::fs::write_string_to_file(data, tempfile);
    std::string data2;
    util::fs::read_file_to_string(tempfile, &data2);
    EXPECT_EQ(data, data2);
}
