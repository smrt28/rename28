#include "utils.h"
#include "error.h"
namespace s28 {
namespace utils {
void sanitize_filename(const std::string &fname) {
    if (fname == "." || fname == ".." || fname.empty()) {
        RAISE_ERROR("sanitize_filename failed 0");
    }

    for (const char c: fname) {
        if (c == '/')
            RAISE_ERROR("sanitize_filename failed 1");
    }
}
}
}
