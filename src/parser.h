#ifndef SRC_PARSER_H
#define SRC_PARSER_H

#include <ctype.h>
#include <string>

#include "error.h"

namespace s28 {
namespace parser {

class Error : public s28::Error {
    public:
        enum Code_t {
            UNDEFINED = 1,
            OVERFLOW  = 2,
            RANGE     = 3,
            EXPECT    = 4
        };

        explicit Error(Code_t code, std::string message = "parslet"):
            s28::Error(message),
            code(code)
        {}
        Code_t code;
};


class Parslet {
public:
    static const int eof = -1;

    Parslet(const char *it, const char *eit) :
        it(it), eit(eit)
    {}

    Parslet(const std::string &s) :
        it(s.c_str()), eit(s.c_str() + s.size())
    {}

    void reset(const std::string &s) {
        it = s.c_str();
        eit = it + s.size();
    }


    std::string str() const {
        return std::string(it, eit);
    }


    const char * begin() const { return it; }
    const char * end() const { return eit; }

    int operator[](ssize_t i) const {
        const char *c;
        if (i >= 0) {
            c = it + i;
        } else {
            c = eit + i;
        }
        if (c >= eit || c < it) return eof;
        return (unsigned char)*c;
    }

    char at(size_t i) const {
        int rv = (*this)[i];
        if (rv == eof) raise(Error::RANGE);
        return rv;
    }

    int next() {
        if (it >= eit) return eof;
        int rv = *it;
        ++it;
        return (unsigned char)rv;
    }

    int operator++() {
        next();
        return (*this)[0];
    }

    int operator++(int) {
        return next();
    }

    void operator+=(size_t i) {
        if (it + i > eit) raise(Error::OVERFLOW);
        it += i;
    }

    operator bool() const {
        return it < eit;
    }

    int operator*() const {
        return at(0);
    }

    void expect_eof() {
        if (!empty()) raise(Error::EXPECT);
    }

    void expect_char(char c) {
        int k = (*this)[0];
        if (k == -1) raise(Error::OVERFLOW);

        if ((char)k != c) raise(Error::EXPECT);
        next();
    }

    void skip() {
        if (it >= eit) raise(Error::OVERFLOW);
        ++it;
    }

    int last() const {
        if (it >= eit) return eof;
        return eit[-1];
    }

    bool shift() {
        if (it < eit) {
            --eit;
            return true;
        }
        return false;
    }

    size_t size() const {
        if (it > eit) raise(Error::OVERFLOW);
        return eit - it;
    }

    bool empty() const { return it == eit; }


private:
    void raise(Error::Code_t code) const {
        throw Error(code);
    }

    const char *it;
    const char *eit;
};

inline void ltrim(Parslet &p) {
    while(p && isspace(*p)) {
        p++;
    }
}

inline void rtrim(Parslet &p) {
    while(isspace(p.last())) {
        p.shift();
    }
}


inline void trim(Parslet &p) {
    ltrim(p);
    rtrim(p);
}

inline std::string qu(Parslet &p) {
    p.expect_char('"');
    std::string rv;
    for (;;) {
        if (*p == '"') {
            p.skip();
            return rv;
        }


        if (*p == '\\') {
            rv += p.at(1);
            p += 2;
            continue;
        }

        rv += p++;
    }
}

inline Parslet split(Parslet &p, char c) {
    Parslet orig = p;
    const char *it = p.begin();
    for (;;) {
        int x = p.next();
        if (x == Parslet::eof) {
            p = orig;
            throw Error(Error::EXPECT);
        }
        if ((unsigned char)x == (unsigned char)c) {
            return Parslet(it, p.begin() - 1);
        }
    }
}

inline std::pair<std::string, std::string> eq(Parslet &p) {
    std::pair<std::string, std::string> rv;
    ltrim(p);
    if (*p == '"') {
        rv.first = qu(p);
        ltrim(p);
        p.expect_char('=');
    } else {
        Parslet l = split(p, '=');
        rtrim(l);
        rv.first = l.str();
    }
    ltrim(p);

    if (p[0] == '"') {
        rv.second = qu(p);
    } else {
        trim(p);
        rv.second = p.str();
    }
    return rv;
}

} // nemaspace parser
} // namespace s28

#endif /* SRC_PARSER_H */
