#include <iostream>

#include "gtest/gtest.h"
#include "escape.h"
#include "filename_parser.h"
#include "parser.h"
#include "utf8.h"
#include "transformer.h"

/*
void check(const std::string &s) {
    EXPECT_EQ(s28::shellunescape(s28::shellescape(s)), s);
}
*/

TEST(Parsing, Escape) {
    using namespace s28;
    /*
    check("$abc");
    check("./Blahopřeji'-अभिनंदन-мекунем-恭喜啦");
    check("");
    check("a");
    check("{a}");
    check("${a}");
    check("''");
    check("''c'''");
    check("ab#$a");
    */

//    EXPECT_EQ(s28::shellunescape("\\ \\$")," $");
//    EXPECT_EQ(s28::shellunescape("\\a"),"a");

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
//    EXPECT_THROW(s28::shellunescape("aaa\\"), std::exception);
}


TEST(Parsing, FileNameParser) {
    using namespace s28;
    GlobalRenameContext gc;
    RenameParserContext ctx(gc);

    {
    s28::FileNameParser fnp("%n_%N.%e");
    std::string s = "abc.xyz";
    EXPECT_EQ(fnp.parse(s, ctx), "abc_1.xyz");
    EXPECT_EQ(fnp.parse(s, ctx), "abc_2.xyz");
    EXPECT_EQ(fnp.parse(s, ctx), "abc_3.xyz");
    EXPECT_EQ(fnp.parse(s, ctx), "abc_4.xyz");
    }

    {
    s28::FileNameParser fnp("%n_%3N.%e");
    std::string s = "abc.xyz";
    EXPECT_EQ(fnp.parse(s, ctx), "abc_001.xyz");
    EXPECT_EQ(fnp.parse(s, ctx), "abc_002.xyz");
    EXPECT_EQ(fnp.parse(s, ctx), "abc_003.xyz");
    EXPECT_EQ(fnp.parse(s, ctx), "abc_004.xyz");
    }

    {
    s28::FileNameParser fnp("%un_%3N.%e");
    std::string s = "abc.xyz";
    EXPECT_EQ(fnp.parse(s, ctx), "ABC_001.xyz");
    EXPECT_EQ(fnp.parse(s, ctx), "ABC_002.xyz");
    }

    {
    s28::FileNameParser fnp("%ln_%3N.%le");
    std::string s = "aBc.xYZ";
    EXPECT_EQ(fnp.parse(s, ctx), "abc_001.xyz");
    EXPECT_EQ(fnp.parse(s, ctx), "abc_002.xyz");
    }

    {
    s28::FileNameParser fnp("%n%-%N.%e");
    std::string s = "abcxyz";
    EXPECT_EQ(fnp.parse(s, ctx), "abcxyz-1.");
    EXPECT_EQ(fnp.parse(s, ctx), "abcxyz-2.");
    }

    {
    s28::FileNameParser fnp("%-%n_%N%.%e");
    std::string s = "abcxyz";
    EXPECT_EQ(fnp.parse(s, ctx), "abcxyz_1");
    EXPECT_EQ(fnp.parse(s, ctx), "abcxyz_2");
    }
}


TEST(Parsing, utf8) {
    using namespace s28;
    std::string text = "今天周五123 abc@#$%(^&*(zB9";

    const char * it = text.c_str();
    const char * eit = text.c_str() + text.size();
    uint32_t cp = 0;
    parser::Parslet p(text);
    while (!p.empty()) {
        cp *= 7691;
        cp += utf8::next(p);
    }
    EXPECT_EQ(cp, 2334994564); // checksum
}

TEST(Parsing, Parslet) {
    using namespace s28;

    std::string text = "12345";
    parser::Parslet p(text);
    for (int i = 0; i < text.size(); ++i) {
        EXPECT_EQ(p[i], '1' + i);
    }
    EXPECT_EQ(p[-1], '5');
    EXPECT_EQ(p[-2], '4');
    EXPECT_EQ(p[6], -1);
    EXPECT_EQ(p[7], -1);
    EXPECT_EQ(p[100], -1);
    EXPECT_EQ(p.str(), text);
}

/*
TEST(Parsing, TotalEscape) {
    using namespace s28;
    std::ostringstream oss;
    std::string s;

    for (int i = 1; i < 256; ++i) {
        oss << (char)i;
    }

    std::cout << shellescape(oss.str(), true) << std::endl;
}
*/
