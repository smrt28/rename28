#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <sstream>


#include "error.h"

namespace s28 {

namespace {
std::string esc_chars[256];

int hex2int(char c) {
    switch(c) {
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;
        case 'A': return 10;
        case 'a': return 10;
        case 'B': return 11;
        case 'b': return 11;
        case 'C': return 12;
        case 'c': return 12;
        case 'D': return 13;
        case 'd': return 13;
        case 'E': return 14;
        case 'e': return 14;
        case 'F': return 15;
        case 'f': return 15;
    }
    throw 1;
}

} // namespace

void init_escaping() {
    const char *hex = "0123456789ABCDEF";
    for (int i = 0; i < 256; ++i) {
        if (isprint(i)) {
            char buf[2];
            buf[0] = i;
            buf[1] = 0;
            esc_chars[i] = buf;
        } else {
            esc_chars[i] = "\\x";
            esc_chars[i] += hex[i >> 4];
            esc_chars[i] += hex[i & 0xf];
        }
    }

    esc_chars[':'] = "\\:";
    esc_chars['\n'] = "\\n";
    esc_chars['\t'] = "\\t";
    esc_chars['\r'] = "\\r";
    esc_chars['\\'] = "\\\\";
}

std::string escape(const std::string &s, bool spaces) {
    std::string rv;

    for (char c: s) {
        if (spaces && c == ' ') {
            rv += "\\ ";
            continue;
        }
        rv += esc_chars[(unsigned char )c];
    }
    return rv;
}

std::string unescape(const std::string &s) {
    std::string rv;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\') {
            if (i == s.size() - 1) RAISE_ERROR("unescape failed");
            switch (s[i + 1]) {
                case 'n':
                    i += 1; rv += "\n";
                    break;
                case 't':
                    i += 1; rv += "\t";
                    break;
                case 'r':
                    i += 1; rv += "\r";
                    break;
                case '\\':
                    i += 1; rv += "\\";
                    break;
                case 'x':
                    if (i + 3 >= s.size()) RAISE_ERROR("unescape failed");;
                    rv += (char)( (hex2int(s[i + 2]) << 4) + hex2int(s[i + 3]));
                    i+=3;
                    break;
                default:
                    rv += s[i + 1]; i++;
                    break;
            }

        } else {
            rv += s[i];
        }
    }
    return rv;
}

std::string base26encode(uint32_t n, int align) {
    static const char *abc = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::string rv;
    while(n > 0 || align > 0)
    {
        align --;
        rv += abc[n % 26];
        n/=26;
    }
    return rv;
}

int base26suggest_alignment(uint32_t n) {
    int rv = 0;
    while(n) {
        rv++;
        n/=26;
    }
    return rv;
}

std::string str_align(const std::string s, size_t len) {
    if (len <= s.size()) return s;
    std::string rv = s;
    for (size_t i = 0; i < len - s.size(); ++i) {
        rv += " ";
    }

    return rv;
}

} // namespace s28
