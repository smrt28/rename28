#ifndef DIR_H
#define DIR_H

#include <vector>
#include <memory>
#include "node.h"

namespace s28 {

class Dir : public Node {
public:
    Dir(Config &config, const std::string &name, Dir *parent) :
        Node(config),
        name(name),
        parent(parent)
    {}

    virtual std::string get_path() const override;
    void build(Config &) override;
    void traverse(Traverse &t) const override;

private:
    std::vector<std::unique_ptr<Node>> children;
    std::string name;
    Dir *parent = nullptr;
};

} // namespace s28

#endif /* DIR_H */
