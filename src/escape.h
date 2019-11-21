#ifndef ESCAPE_H
#define ESCAPE_H

#include <string>

namespace s28 {


// Escapes string to be used in the shell command line. If hardened == false,
// it throws on ctrl characters. If hardened == true, it can handle any
// mess in the string, but the result would be really ugly.
std::string shellescape(const std::string &s, bool hardened = false);

// Unescapes string escaped by shellescape(s, false) function. Wont work with hardened true.
std::string shellunescape(const std::string &s);

std::string base26encode(uint32_t, int align = 1);
int base26suggest_alignment(uint32_t n);
std::string str_align(const std::string s, size_t len);

}

#endif
