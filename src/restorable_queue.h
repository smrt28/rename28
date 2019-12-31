#ifndef RESTORABLE_QUEUE_H
#define RESTORABLE_QUEUE_H

#include <vector>

namespace s28 {

template<typename T>
class RestorableQueues {
//    bool dirty = false;
    typedef std::vector<std::unique_ptr<T>> Queue;
    std::vector<Queue> queues;

public:
    // state is vector of queue sizes
    typedef std::vector<size_t> State;

    // push an item to the queue specified by id
    void push(size_t id, T *item) {
//        dirty = true;
        queues.resize(std::max(id + 1, queues.size()));
        queues[id].push_back(std::unique_ptr<T>(item));
    }

    // returns top item in queue specified by id 
    T * operator[] (size_t qid) {
        if (qid >= queues.size()) return nullptr;
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

            while (q.size() > st[i]) {
                q.pop_back();
            }
        }
    }

    // return number of queues
    size_t queues_count() const {
        return queues.size();
    }
};

} // namespace s28

#endif /* RESTORABLE_QUEUE_H */
