#ifndef PATH_CONTEXT_H
#define PATH_CONTEXT_H

#include <memory>
#include <functional>
#include <deque>
#include "transformer.h"
namespace s28 {

class PathContext {
    std::unique_ptr<PathBuilder> path_builder;
    std::deque<std::unique_ptr<PathBuilder>> queue;

public:
    PathContext() {}

    typedef std::function<void()> RevertFn;
    typedef std::vector<RevertFn> RevStack;
    typedef std::vector<std::string> DirChain;

    size_t size() const { return queue.size(); }

    void push(PathBuilder *dirpath) {
        queue.push_front(std::unique_ptr<PathBuilder>(dirpath));
    }

    bool build(const DirChain &dirchain, std::string &path) {
        DirChain src = dirchain; // TODO optimize
        DirChain dst;
        for (std::unique_ptr<PathBuilder> &p: queue) {
            switch (p->build(src, dst)) {
                case PathBuilder::SKIP:
                    return false;
                case PathBuilder::UNCHANGED:
                    break;
                case PathBuilder::CHANGED:
                    std::swap(src, dst);
                    break;
            }
        }

        path = boost::algorithm::join(src, "/");
        return true;
    }

    void pop_to(size_t len) {
        while(queue.size() > len) queue.pop_front();
    }
};

} // namespace s28
#endif
