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

#include "error.h"
#include "escape.h"
#include "string.h"
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
    RAISE_ERROR("hex2int failed");
}

} // namespace

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


void Escaper::escchar(char c, char *out, bool qu) {
    static const char *hex = "0123456789ABCDEF";
    switch(c) {
        case '\n': out[0] = '\\'; out[1] = 'n'; out[2] = 0; return;
        case '\r': out[0] = '\\'; out[1] = 'r'; out[2] = 0; return;
        case '\t': out[0] = '\\'; out[1] = 't'; out[2] = 0; return;
        case '\\': out[0] = '\\'; out[1] = '\\'; out[2] = 0; return;
    }

    if (!qu) {
        switch(c) {
            case '{': out[0] = '\\'; out[1] = '{'; out[2] = 0; return;
            case '}': out[0] = '\\'; out[1] = '}'; out[2] = 0; return;
            case ' ': out[0] = '\\'; out[1] = ' '; out[2] = 0; return;
            case '#': out[0] = '\\'; out[1] = '#'; out[2] = 0; return;
            case '$': out[0] = '\\'; out[1] = '$'; out[2] = 0; return;
        }
    }

    if (c == '\'') {
        out[0] = '\\'; out[1] = '\''; out[2] = 0; return;
    }

    if (!isprint(c)) {
        unsigned char i = c;
        out[0] = '\\';
        out[1] = 'x';
        out[2] += hex[i >> 4];
        out[3] += hex[i & 0xf];
        out[4] = 0;
        return;
    }

    out[0] = c;
    out[1] = 0;
}


std::string Escaper::escape(const std::string &s) {
    std::string rv;
    bool q = false;

    for (char c: s) {
        if (isspace(c)) {
            q = true;
            break;
        }

        if (c == '*' || c =='&')
            q = true;
    }

    if (q) rv = "'";
    char buf[8];
    memset(buf, 0, sizeof(buf));
    for (char c: s) {
        escchar(c, buf, q);
        rv += buf;
    }
    if (q) rv += "'";

    return rv;
}

std::string Escaper::unescape(const std::string &s) {
    std::string rv;
    size_t i = 0;
    size_t size = s.size();

    if (s.size() >= 2 && s[0] == '\'' && s[size - 1] == '\'') {
        i++; size--;
    }

    for (; i < size; ++i) {
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

} // namespace s28
