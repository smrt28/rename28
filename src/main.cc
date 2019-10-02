#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>


#include <set>
#include <map>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <openssl/sha.h>
#include <boost/algorithm/string/join.hpp>

#include <boost/bind.hpp>

#include "node.h"
#include "dir.h"

#include "escape.h"
#include "error.h"
#include "hash.h"
#include "file.h"


namespace s28 {




namespace aux {
template<typename CB>
class Collector : public Traverse {
public:
    Collector(const Node *node, CB cb) : node(node), cb(cb) {}
    void walk(const Node *node) override {
        cb(node);
    }

private:
    const Node *node;
    CB cb;
};
}

template<typename CB>
void walk(const Node *node, CB cb) {
    aux::Collector<CB> collector(node, cb);
    node->traverse(collector);
}


class Collector {
    std::map<std::string, std::set<ino_t> > dups;

    struct Record {
        std::string hash;
        const Node *node = nullptr;
        ino_t inode = 0;
        int links_total;
        int links = -1;
    };
public:

    void index(const Node *n) {
        records[n->get_id()].node = n;
    }

    void hash(const Node *n) {
        const File *f = dynamic_cast<const File *>(n);
        if (!f) return;
        std::cerr << "hashing: " << f->get_path() << std::endl;
        records[f->get_id()].hash = hash_file(f->get_path());
    }

    void stat(const Node *n) {
        struct stat stt;
        if (::stat(n->get_path().c_str(), &stt) == -1) {
            RAISE_ERROR("stat failed; file=" << n->get_path());
        }
        records[n->get_id()].inode = stt.st_ino;
    }

    void find_duplicates() {
        for (auto const &r: records) {
            dups[r.second.hash].insert(r.second.inode);
        }
    }

    void find_links() {
        std::map<ino_t, int> links;
        for (auto const &r: records) {
            auto link = links.find(r.second.inode);
            if (link == links.end()) {
                links[r.second.inode] = 0;
            } else {
                if (link->second == 0)
                    link->second ++;
                link->second ++;
            }
        }

        for (auto &r: records) {
            r.second.links_total = links[r.second.inode];
        }

        for (auto &r: records) {
            auto link = links.find(r.second.inode);
            r.second.links = link->second;
            link->second --;
        }
    }


    void dump() {

        size_t max_path_len = 0;
        for (auto &r: records) {
            max_path_len = std::max(r.second.node->get_path().size(), max_path_len);
        }

        for (auto &r: records) {
            if (!dynamic_cast<const File *>(r.second.node)) continue;

            std::cout << str_align(escape(r.second.node->get_path(), true), max_path_len + 1) <<
                " #" << r.second.inode;
           
            if (r.second.links > 0) {
                std::cout<< ".(" << r.second.links << "/" << r.second.links_total << ")";
            }

            const std::set<ino_t> &duplicates = dups[r.second.hash];

            if (duplicates.size() > 1) {
                std::vector<std::string> dups;
                for (auto inode: duplicates) {
                    if (inode == r.second.inode) continue;
                    dups.push_back(std::to_string(inode));
                }

                std::string joined = boost::algorithm::join(dups, ", ");
                std::cout << "; " << joined;
            }

            std::cout << std::endl;
        }
    }


private:
    std::map<uint32_t, Record> records;
};


class MultiBind {
    public:
        class Call {
            public:
                virtual void call() {};
        };

        MultiBind(int threads, Call call) : threads(threads), call(call) {}
    private:
        int threads;
        Call call;
};


} // namespace s28


int main() {
    s28::init_escaping();

    s28::Node::Config config;
    s28::Dir d(config, ".", nullptr);
    d.build(config);
    s28::Collector collector;

    s28::walk(&d, boost::bind(&s28::Collector::index, &collector, _1));
    s28::walk(&d, boost::bind(&s28::Collector::hash, &collector, _1));
    s28::walk(&d, boost::bind(&s28::Collector::stat, &collector, _1));

    collector.find_duplicates();
    collector.find_links();
    collector.dump();

    return 0;
}

