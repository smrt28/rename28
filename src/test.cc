#include <iostream>

#include "gtest/gtest.h"
#include "escape.h"

bool check(s28::Escaper &es, const std::string &s) {
    return (es.unescape(es.escape(s)) == s);
}


TEST(Dummy, Escape) {
    using namespace s28;

    Escaper::Config escconfig;
    Escaper es(escconfig);

    try {
        EXPECT_TRUE(check(es, "asb\xF1" "1\"  g"));
        EXPECT_TRUE(check(es, ""));
        EXPECT_TRUE(check(es, "\t\r\n \"\'"));
        EXPECT_TRUE(check(es, "\t\r\n \\xFF\\xF0\"\'"));
    } catch (const std::exception &e) {
        std::cout << e.what() << std::endl;
    }
}

