#include <iostream>

#include "gtest/gtest.h"
#include "escape.h"
#include "filename_parser.h"

bool check(s28::Escaper &es, const std::string &s) {
    return (es.unescape(es.escape(s)) == s);
}


TEST(Parsing, Escape) {
    using namespace s28;

    Escaper es;

    EXPECT_TRUE(check(es, "asb\xF1" "1\"  g"));
    EXPECT_TRUE(check(es, ""));
    EXPECT_TRUE(check(es, "a"));
    EXPECT_TRUE(check(es, "{a}"));
    EXPECT_TRUE(check(es, "''"));
    EXPECT_TRUE(check(es, "''c'''"));
    EXPECT_TRUE(check(es, "ab"));
    EXPECT_TRUE(check(es, "abc"));
    EXPECT_TRUE(check(es, "aasfg'as\\dgagagdasg   # ASDFASDF#ASDFbc"));
    EXPECT_TRUE(check(es, "\t\r\n \"\'"));
    EXPECT_TRUE(check(es, "\t\r\n \\xFF\\xF0\"\'"));
}


TEST(Parsing, FileNameParser) {
    using namespace s28;
    {
    s28::FileNameParser fnp("%n_%N.%e");
    std::string s = "abc.xyz";
    EXPECT_EQ(fnp.parse(s), "abc_1.xyz");
    EXPECT_EQ(fnp.parse(s), "abc_2.xyz");
    EXPECT_EQ(fnp.parse(s), "abc_3.xyz");
    EXPECT_EQ(fnp.parse(s), "abc_4.xyz");
    }

    {
    s28::FileNameParser fnp("%n_%3N.%e");
    std::string s = "abc.xyz";
    EXPECT_EQ(fnp.parse(s), "abc_001.xyz");
    EXPECT_EQ(fnp.parse(s), "abc_002.xyz");
    EXPECT_EQ(fnp.parse(s), "abc_003.xyz");
    EXPECT_EQ(fnp.parse(s), "abc_004.xyz");
    }

    {
    s28::FileNameParser fnp("%un_%3N.%e");
    std::string s = "abc.xyz";
    EXPECT_EQ(fnp.parse(s), "ABC_001.xyz");
    EXPECT_EQ(fnp.parse(s), "ABC_002.xyz");
    }

    {
    s28::FileNameParser fnp("%ln_%3N.%le");
    std::string s = "aBc.xYZ";
    EXPECT_EQ(fnp.parse(s), "abc_001.xyz");
    EXPECT_EQ(fnp.parse(s), "abc_002.xyz");
    }


    {
    s28::FileNameParser fnp("%n_%N.%e");
    std::string s = "abcxyz";
    EXPECT_EQ(fnp.parse(s), "abcxyz_1.");
    EXPECT_EQ(fnp.parse(s), "abcxyz_2.");
    }
}
