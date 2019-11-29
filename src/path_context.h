#ifndef PATH_CONTEXT_H
#define PATH_CONTEXT_H

#include <memory>
#include <functional>
#include <deque>
#include "transformer.h"
namespace s28 {

template<typename T>
class RestorableQueues {
    bool dirty = false;
    typedef std::vector<std::unique_ptr<T>> Queue;
    std::vector<Queue> queues;

public:
    // state is vector of queue sizes
    typedef std::vector<size_t> State;

    // push an item to the queue specified by id
    void push(size_t id, T *item) {
        dirty = true;
        queues.resize(std::max(id + 1, queues.size()));
        queues[id].push_back(std::unique_ptr<T>(item));
    }

    // returns top of queue specified by id
    T * operator[] (size_t qid) {
        Queue & q = queues[qid];
        if (q.empty()) return nullptr;
        return q.back().get();
    }

    State state() const {
        State rv;
        for (size_t i = 0; i < queues_count(); ++i) {
            rv.push_back( queues[i].size() );
        }
        return rv;
    }


    // restore queue sizes by popping from the top
    void restore(const State &st) {
        for (size_t i = 0; i < queues_count(); ++i) {
            Queue & q = queues[i];
            if (i >= st.size()) {
                q.clear();
                continue;
            }

            while (q.size() >st[i]) {
                q.pop_back();
            }
        }
    }

    // return current number of queues
    size_t queues_count() const {
        return queues.size();
    }
};


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
