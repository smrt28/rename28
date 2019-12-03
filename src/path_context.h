#ifndef PATH_CONTEXT_H
#define PATH_CONTEXT_H

#include <memory>
#include <functional>
#include <deque>

#include "transformer.h"
#include "restorable_queue.h"

namespace s28 {


class PathContext {
public:
    PathContext() {}

    typedef RestorableQueues<PathBuilder> Queues;
    typedef Queues::State State;

    Queues path_bulders;

    typedef std::vector<std::string> DirChain;

    void push(int id, PathBuilder *dirpath) {
        path_bulders.push(id, dirpath);
    }

    State state() const { return path_bulders.state(); }
    void restore(const State &st) { path_bulders.restore(st); }

    bool build(const DirChain &dirchain, DirChain &path, RenameParserContext &ctx, int dups) {
        DirChain src = dirchain;
        DirChain dst;

        // for all the queues
        for (size_t id = 0; id < path_bulders.queues_count(); ++id) {
            PathBuilder *builder = path_bulders[id];
            if (!builder) continue; // skip empty queue
            dst.clear();
            switch (builder->build(src, dst, ctx, dups)) {
                case PathBuilder::SKIP: // this directory/file will not be created
                    return false;
                case PathBuilder::UNCHANGED: // keep the dir
                    break;
                case PathBuilder::CHANGED:
                    std::swap(src, dst);
                    break;
            }
        }

        std::swap(path, src);
        return true;
    }
};

} // namespace s28
#endif
