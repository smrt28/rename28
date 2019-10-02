#ifndef NODE_H
#define NODE_H

#include <stdint.h>
#include <string>

namespace s28 {

class Node;

class Traverse {
public:
    virtual void walk(const Node *) = 0;
};

class Node {
public:
    class Config {
    public:
        uint32_t get_next_id() {
            return ++next_id;
        }
    private:
        uint32_t next_id = 0;
    };

    Node(Config &config) :
        id(config.get_next_id())
    {}

    virtual ~Node() {}

    virtual void build(Config &) = 0;
    virtual void traverse(Traverse &t) const = 0;
    virtual std::string get_path() const = 0;

    uint32_t get_id() const { return id; }

protected:
    uint32_t id;
};


} // namespace s28

#endif /* NODE_H */
