#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <algorithm>
#include <deque>
#include "error.h"
#include "escape.h"
#include "string.h"
#include "parser.h"
#include "utf8.h"
namespace s28 {

namespace {
std::string esc_chars[256];
/*
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
    RAISE_ERROR("hex2int failed");
}
*/
bool is_ctl(uint32_t code) {
    // https://en.wikipedia.org/wiki/C0_and_C1_control_codes
    if (code < 0x20 ||
            (code >= 0x80 && code <= 0x9f)) return true;
    return false;
}


} // namespace


std::string shell_soft_escape(const std::string &s) {
    std::vector<unsigned short> utf16line;
    auto end_it = utf8::find_invalid(s.begin(), s.end());
    if (end_it != s.end()) {
        RAISE_ERROR("Invalid UTF-8 encoding detected");
    }

    utf8::utf8to16(s.begin(), end_it, std::back_inserter(utf16line));

    std::deque<unsigned short> utf16rv;
    bool need_quotes = false;
    for (unsigned short c: utf16line) {
        if (is_ctl(c)) {
            RAISE_ERROR("control character detected");
        }

        if (c == '"' || c == '$' || c == '\\' || c == '`') {
            utf16rv.push_back('\\');
        }
        if (c == ' ' || c == '&' || c == '\'') need_quotes = true;

        utf16rv.push_back(c);
    }

    if (need_quotes) {
        utf16rv.push_front('"');
        utf16rv.push_back('"');
    }

    std::string rv;
    utf8::utf16to8(utf16rv.begin(), utf16rv.end(), std::back_inserter(rv));
    return rv;
}


namespace {
void escape_mess(std::vector<char> &mess, std::vector<char> &rv) {
    static const char * shellprintf = "$(printf '";
    static const char *abc = "0123456789ABCDEF";

    if (mess.empty()) return;
    for (const char *ch = shellprintf;*ch; ++ch) rv.push_back(*ch);

    for (char c: mess) {
        rv.push_back('\\');
        rv.push_back('x');
        rv.push_back(abc[(uint8_t)c >> 4]);
        rv.push_back(abc[(uint8_t)c & 0xf]);
    }

    rv.push_back('\'');
    rv.push_back(')');
    mess.clear();
}

bool is_hardened(const std::string &s) {
    if (s.size() < 3) return false;
    if (s[0] != '"') return false;
    if (s.back() != '"') return false;


    return true;
}

}

std::string shell_hard_escape(const std::string &s) {
    std::vector<char> rv;
    rv.push_back('"');

    std::vector<char> mess;
    for (char c: s) {
        if (!isprint(c)) {
            mess.push_back(c);
            continue;
        } else {
            if (!mess.empty()) {
                escape_mess(mess, rv);
            }
        }
        switch(c) {
            case '`':
            case '"':
            case '$':
                rv.push_back('\\');
                rv.push_back(c);
                break;
            default:
                rv.push_back(c);
                break;
        }
    }

    escape_mess(mess, rv);
    rv.push_back('"');
    return std::string(rv.begin(), rv.end());

}

std::string shellescape(const std::string &s, bool hardened) {
    if (hardened) {
        try
        {
            return shell_soft_escape(s);
        } catch(...) {
            return shell_hard_escape(s);
        }
    }
    return shell_soft_escape(s);
}

std::string shellunescape(const std::string &s) {
    parser::Parslet p(s);
    return read_escaped_string(p);
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
