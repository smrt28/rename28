#ifndef ERROR_H
#define ERROR_H

#include <string>
#include <exception>

#define RAISE_ERROR(msg) do { std::ostringstream self; self << msg; throw Error(self.str()); } while(0)

namespace s28 {
class Error : public std::exception
{
public:
    Error(const std::string &msg) :
        msg(msg)
    {}

    virtual ~Error() throw() {}

    const char* what() const throw() {
        return msg.c_str();
    }

private:
    std::string msg;
};
}

#endif
