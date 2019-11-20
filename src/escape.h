#ifndef ESCAPE_H
#define ESCAPE_H

#include <string>

namespace s28 {

class Escaper {
public:
    std::string escape(const std::string &);

private:
    void escchar(char c, char *out, bool qu);
};

std::string shellescape(const std::string &s);
std::string shellunescape(const std::string &s);

std::string base26encode(uint32_t, int align = 1);
int base26suggest_alignment(uint32_t n);
std::string str_align(const std::string s, size_t len);

}

#endif
