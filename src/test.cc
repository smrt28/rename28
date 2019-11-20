#include <iostream>

#include "gtest/gtest.h"
#include "escape.h"
#include "filename_parser.h"
#include "parser.h"
#include "utf8.h"

bool check(s28::Escaper &es, const std::string &s) {
    return (es.unescape(es.escape(s)) == s);
}


TEST(Parsing, Escape) {
    using namespace s28;

    Escaper es;

    EXPECT_TRUE(check(es,"./Blahopřeji'-अभिनंदन-мекунем-恭喜啦"));
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


    std::string text = "今天周五123 abc@#$%(^&*(zA9";
    EXPECT_EQ(s28::shellescape(text), "\"今天周五123 abc@#\\$%(^&*(zA9\"");

    text = "sabdHVBJF42378y234987!@#%^*";
    EXPECT_EQ(s28::shellescape(text), text);

    text = "sabdHVBJF4237&8y234987!@#%^*"; // & causes quotes
    EXPECT_EQ(s28::shellescape(text), std::string("\"") + text + "\"");

    text = "sabdHVBJ'F4237"; // ' causes quotes
    EXPECT_EQ(s28::shellescape(text), std::string("\"") + text + "\"");

    text = "sabdHVBJF4237";
    EXPECT_EQ(s28::shellescape(text), text);

    text = "sabdHVB\\JF4237";
    EXPECT_EQ(s28::shellescape(text), "sabdHVB\\\\JF4237");

    EXPECT_THROW(s28::shellescape("\t"), std::exception);
    EXPECT_THROW(s28::shellescape("\n"), std::exception);
    EXPECT_THROW(s28::shellescape("\r"), std::exception);
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
    s28::FileNameParser fnp("%n%-%N.%e");
    std::string s = "abcxyz";
    EXPECT_EQ(fnp.parse(s), "abcxyz-1.");
    EXPECT_EQ(fnp.parse(s), "abcxyz-2.");
    }

    {
    s28::FileNameParser fnp("%-%n_%N%.%e");
    std::string s = "abcxyz";
    EXPECT_EQ(fnp.parse(s), "abcxyz_1");
    EXPECT_EQ(fnp.parse(s), "abcxyz_2");
    }
}


TEST(Parsing, Parslet) {
    using namespace s28;
    std::string text = "今天周五123 abc@#$%(^&*(zB9";


    uint32_t cp = 0;
    const char * it = text.c_str();
    const char * eit = text.c_str() + text.size();

    parser::Parslet p(text);

    while (!p.empty()) {
    //    std::cout << p.next_utf() << std::endl;
    }
}

