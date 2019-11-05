#ifndef ESCAPE_H
#define ESCAPE_H

#include <string>

namespace s28 {

class Escaper {
public:
    class Config {
    public:
        bool quotes1 = true;
        bool quotes2 = true;
        bool spaces = true;
    };

    Escaper(const Config &config) : config(config) {}

    std::string escape(const std::string &);
    std::string unescape(const std::string &);

private:
    void escchar(char c, char *out);
    const Config config;
};

void init_escaping();

std::string escape(const std::string &s, bool spaces = true);
std::string unescape(const std::string &s);

std::string base26encode(uint32_t, int align = 1);
int base26suggest_alignment(uint32_t n);
std::string str_align(const std::string s, size_t len);

}

#endif
