#ifndef NODE_H
#define NODE_H

#include <stdint.h>
#include <string>

namespace s28 {

class Node;

class Traverse {
public:
    virtual void walk(const Node *) {}
    virtual void on_file(const Node *) {}
    virtual void on_dir_begin(const Node *) {}
    virtual void on_dir_end(const Node *) {}
};

class Node {
public:
    class Config {
    public:
    };

    Node(Config &config)
    {}

    virtual ~Node() {}

    virtual void build(Config &) = 0;
    virtual void traverse(Traverse &t) const = 0;
    virtual std::string get_path() const = 0;
    virtual std::string get_name() const = 0;
    virtual Node * get_parent() = 0;


    template <typename T>
    bool is() const {
        if (dynamic_cast<const T *>(this)) return true;
        return false;
    }
};


} // namespace s28

#endif /* NODE_H */
