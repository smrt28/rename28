#ifndef DIR_H
#define DIR_H

#include <vector>

#include "node.h"

namespace s28 {

class Dir : public Node {
public:
    Dir(Config &config, const std::string &path, Dir *parent) :
        Node(config, path),
        parent(parent)
    {}

    void build(Config &) override;
    void traverse(Traverse &t) const override;

private:
    std::vector<std::unique_ptr<Node>> children;
    Dir *parent = nullptr;
};

} // namespace s28

#endif /* DIR_H */
