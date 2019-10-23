#ifndef FILE_H
#define FILE_H

#include "node.h"
#include "dir.h"
namespace s28 {
class File : public Node {
public:
    File(Config &config, const std::string &name, Dir *parent) :
        Node(config),
        name(name),
        parent(parent)
    { 
        if (!parent) throw; // not reachable, this is assert 
    }

    std::string get_path() const override;
    void build(Config &) override;
    void traverse(Traverse &t) const override;

    std::string get_name() const override { return name; }
    Node * get_parent() { return parent; }
private:
    std::string name;
    Dir *parent = nullptr;
};
} // namespace s28

#endif
