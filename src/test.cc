#include <iostream>

#include "gtest/gtest.h"
#include "escape.h"

TEST(Dummy, Escape) {
    using namespace s28;
    Escaper::Config escconfig;
    Escaper es(escconfig);

    std::cout << es.escape("asb\xF1" "1\"g") << std::endl;


}

