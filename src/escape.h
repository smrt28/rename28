#ifndef ESCAPE_H
#define ESCAPE_H

#include <string>

namespace s28 {

void init_escaping();
std::string escape(const std::string &s);
std::string unescape(const std::string &s);


}

#endif
