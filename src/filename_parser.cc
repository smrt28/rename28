
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <sstream>
#include "filename_parser.h"
#include "parser.h"
#include "error.h"
namespace s28 {
namespace {

int dec2int(char c) {
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
    }
    RAISE_ERROR("dec2int failed");

    return 0;
}


void check_wildcard_char(char c) {
    switch (c) {
        case 'e': // extension
        case 'n': // file name
        case 'N': // number
        case '.': // '.', if it's not last character
        case '-': // '-', if it's not first character
        case '%':
            break;
        default:
            RAISE_ERROR("invalid file patern");
    }
}

std::string set_case(const std::string &s, size_t c) {
    if (c == 'l') {
        return boost::to_lower_copy(s);
    } else if (c == 'u') {
        return boost::to_upper_copy(s);
    } else {
        return s;;
    }
}

} // namespace

// simple patern tokenizer
FileNameParser::FileNameParser(const std::string &rpat) : rawpatern(rpat) {
    parser::Parslet m(rawpatern);
    const char *it = m.begin();
    while(!m.empty()) {
        if (*m == '%') {
            // append substring to patern vector
            if (it != m.begin()) {
                Token t;
                t.type = TOKEN_STRING;
                t.str.it = it;
                t.str.eit = m.begin();
                patern.push_back(t);
            }

            uint32_t n = 0;

            m.skip();
            switch(*m) {
                case 'l':
                case 'u':
                    n = *m;
                    m.skip();
                    break;
            }

            if (n == 0) {
                // a number can prefix the wildcard
                while (isdigit(*m)) {
                    n = (n * 10) + dec2int(*m);
                    m.skip();
                }
            }


            // check allowed wildcard (after %) character
            check_wildcard_char(*m);
            Token t;
            t.type = TOKEN_WILDCARD;
            t.wld.arg = n;
            t.wld.c = *m;
            patern.push_back(t);

            // skip the wildcard char
            m.skip();
            it = m.begin();
        } else {
            m.skip();
        }
    }

    if (it != m.begin()) {
        Token t;
        t.type = TOKEN_STRING;
        t.str.it = it;
        t.str.eit = m.begin();
        patern.push_back(t);
    }
}


namespace {
class StringBuilder {
    public:
        void append(const std::string &s) {
            if (s.empty()) return;
            if (dots) {
                dots = 0;
                oss << ".";
            }
            empty = false;
            oss << s;
        }

        std::string str() {
            return oss.str();
        }

        int dots = 0;
        bool empty = true;
    private:
        std::ostringstream oss;
};

}


std::string FileNameParser::parse(const std::string &fname) {
    parser::Parslet fullname(fname);
    parser::Parslet name = fullname;

    // find last '.' character
    while (!name.empty() && name.shift() != '.');

    parser::Parslet ext;

    // if name is empty, the filename has no extension
    if (name.empty()) {
        name = fullname;
    } else {
        // extension is from last '.' to he end of fullname
        ext = parser::Parslet(name.end() + 1, fullname.end());
    }

    StringBuilder builder;
//    std::ostringstream oss;

    // loop tokens
    for (auto &t: patern) {
        // handle const string token
        if (t.type == TOKEN_STRING) {
            builder.append( std::string(t.str.it, t.str.eit));
            continue;
        }

        // handle wildcard
        switch(t.wld.c) {
            case '%':
                builder.append("%");
                break;
            case 'e':
                builder.append(set_case(ext.str(), t.wld.arg));
                break;
            case 'n':
                builder.append(set_case(name.str(), t.wld.arg));
                break;
            case 'N':
                {
                    std::string s = std::to_string(++cnt);
                    size_t x = std::min(t.wld.arg, size_t(10));
                    if (x > s.size()) {
                        for (size_t i = 0; i < x - s.size(); i++ ) {
                            builder.append("0");
                        }
                    }
                    builder.append(s);
                }
                break;
            case '.':
                builder.dots ++;
                break;
            case '-':
                if (!builder.empty) {
                    builder.append("-");
                }
                break;
        }
    }

    return builder.str();
}


} // namespace s28
