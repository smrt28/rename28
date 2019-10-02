#ifndef FILE_H
#define FILE_H

#include "node.h"

namespace s28 {
class File : public Node {
public:
    using Node::Node;
    void build(Config &) override;
    void traverse(Traverse &t) const override;
};
} // namespace s28

#endif
