#ifndef ESCAPE_H
#define ESCAPE_H

#include <string>

namespace s28 {

void init_escaping();

std::string escape(const std::string &s, bool spaces = true);
std::string unescape(const std::string &s);

std::string base26encode(uint32_t, int align = 1);
int base26suggest_alignment(uint32_t n);
std::string str_align(const std::string s, size_t len);

}

#endif
