#ifndef RECORD_H
#define RECORD_H

#include <boost/core/noncopyable.hpp>
#include <string>

namespace s28 {
class Node;

namespace collector {
class BaseRecord : public boost::noncopyable {
    public:
        const Node *node = nullptr;
        ino_t inode = 0;
        int brackets = 0;
};

class Record : public BaseRecord  {
public:
    std::string hash;
    Record *repre = nullptr;
    Record *next = nullptr;
};

}
}
#endif
