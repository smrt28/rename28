#ifndef FILE_H
#define FILE_H

#include "node.h"

namespace s28 {
class Dir;
class File : public Node {
public:
    File(Config &config, const std::string &path, Dir *parent) :
        Node(config, path),
        parent(parent)
    {}

    void build(Config &) override;
    void traverse(Traverse &t) const override;
private:
    Dir *parent = nullptr;
};
} // namespace s28

#endif
