// Test cases for the file system interface.
// Written by Simon Fuhrmann.

#include <gtest/gtest.h>

#include "util/filesystem.h"

TEST(FileSystemTest, GetFileComponentTest)
{
    EXPECT_EQ("file.txt", util::fs::get_file_component("/tmp/file.txt"));
    EXPECT_EQ("file", util::fs::get_file_component("/tmp/file"));
    EXPECT_EQ("", util::fs::get_file_component("/tmp/file/"));
    EXPECT_EQ("file.txt", util::fs::get_file_component("test/file.txt"));
    EXPECT_EQ("", util::fs::get_file_component(""));
}

TEST(FileSystemTest, GetPathComponentTest)
{
    EXPECT_EQ("/", util::fs::get_path_component("/test"));
    EXPECT_EQ("/test", util::fs::get_path_component("/test/file.txt"));
    EXPECT_EQ(util::fs::get_cwd_string() + "/../dir", util::fs::get_path_component("../dir/file"));
    EXPECT_EQ(util::fs::get_cwd_string(), util::fs::get_path_component(".."));
    EXPECT_EQ(util::fs::get_cwd_string(), util::fs::get_path_component("."));

    // The following tests don't work on OSX and Windows.
    //util::fs::set_cwd("/tmp");
    //EXPECT_EQ("/tmp", util::fs::get_path_component("test.txt"));
    //EXPECT_EQ("/tmp", util::fs::get_path_component(""));
    //EXPECT_EQ("/tmp/blah", util::fs::get_path_component("blah/test.txt"));
}

TEST(FileSystemTest, ReplaceExtensionTest)
{
    EXPECT_EQ("file.bbb", util::fs::replace_extension("file.aaa", "bbb"));
    EXPECT_EQ("file.bbb", util::fs::replace_extension("file", "bbb"));
    EXPECT_EQ("/a/file.b", util::fs::replace_extension("/a/file.a", "b"));
    EXPECT_EQ("/a.b/c.e", util::fs::replace_extension("/a.b/c.d", "e"));
    EXPECT_EQ("/a.b/cd.e", util::fs::replace_extension("/a.b/cd", "e"));
}
