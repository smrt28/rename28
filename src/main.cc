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

class Progress {
public:
    virtual void tick(size_t n, size_t total) {
//        std::cerr << n << "/" << total << std::endl;
    }
};

template<typename CB>
void walk(const Node *node, CB cb) {
    aux::Collector<CB> collector(node, cb);
    node->traverse(collector);
}



namespace collector {

class Record : public boost::noncopyable {
public:
    std::string hash;
    const Node *node = nullptr;
    Record *repre = nullptr;
    Record *next = nullptr;
    ino_t inode = 0;
    int links_total;
    int links = -1;
};

typedef std::map<uint32_t, Record> Records;


void find(const Node *n, Records &records) {
    records[n->get_id()].node = n;
}

void stat(Records &records, Progress &progress) {
    size_t cnt = 0;
    for (auto &rec: records) {
        progress.tick(++cnt, records.size());
        struct stat stt;
        std::string path = rec.second.node->get_path();
        if (::stat(path.c_str(), &stt) == -1) {
            RAISE_ERROR("stat failed; file=" << rec.second.node->get_path());
        }
        rec.second.inode = stt.st_ino;
    }
}

void hash(Records &records, Progress &progress) {
    size_t cnt = 0;
    for (auto &rec: records) {
        progress.tick(++cnt, records.size());
        const Node *n = rec.second.node;
        const File *f = dynamic_cast<const File *>(n);
        if (!f) continue;
        rec.second.hash = hash_file_short(f->get_path());
    }
}


void group_duplicates(Records &records, Progress &progress) {
    size_t cnt = 0;
    std::map<std::string, s28::collector::Record *> uniq;
    for (auto &recit: records) {
        progress.tick(++cnt, records.size());
        auto &rec = recit.second;
        if (rec.hash.empty()) continue;
        auto it = uniq.find(rec.hash);
        if (it == uniq.end()) {
            uniq[rec.hash] = &rec;
        } else {
            s28::collector::Record *repre = it->second->repre;
            if (repre) {
                rec.repre = repre;
                rec.next = repre->next;
            } else {
                repre = it->second;
                repre->repre = rec.repre = repre;
            }
            repre->next = &rec;
        }
    }
}


}

#if 0

class Collector {
    std::map<std::string, std::set<ino_t> > dups;
    Records records;
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
        std::cerr << "stat: " <<  n->get_path() << std::endl;
        struct stat stt;
        if (::stat(n->get_path().c_str(), &stt) == -1) {
            RAISE_ERROR("stat failed; file=" << n->get_path());
        }
        records[n->get_id()].inode = stt.st_ino;
    }

    void find_duplicates() {
        std::cerr << "finding duplicates" << std::endl;
        for (auto const &r: records) {
            dups[r.second.hash].insert(r.second.inode);
        }
    }

    void find_links() {
        std::cerr << "looking for hard links" << std::endl;
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

        std::cerr << "counting hard links" << std::endl;
        for (auto &r: records) {
            r.second.links_total = links[r.second.inode];
        }

        for (auto &r: records) {
            auto link = links.find(r.second.inode);
            r.second.links = link->second;
            link->second --;
        }
    }
/*
    void on_file(const Node *n) {
        for(int i = 0; i < dep; ++i) {
            std::cout << "    ";
        }
        Record &rec = records[n->get_id()];
        const std::set<ino_t> &duplicates = dups[rec.hash];
        std::cout << str_align(s28::escape(n->get_name(), true), 50) << " #" << *duplicates.begin();

        if (duplicates.size() > 1) {
            std::cout << ".DUP";// << duplicates.size();
        }

        std::cout << std::endl;
    }

    void on_dir_begin(const Node *n) {
        for(int i = 0; i < dep; ++i) {
            std::cout << "    ";
        }
        std::cout << n->get_name() << " {" << std::endl;

        dep ++;
    }

    void on_dir_end(const Node *) {
        dep--;
        for(int i = 0; i < dep; ++i) {
            std::cout << "    ";
        }
        std::cout << "}" << std::endl;
    }


private:
    int dep = 0;
    */
};


class InodeCollector : public Traverse {

};
#endif
} // namespace s28

int main() {
    s28::init_escaping();

    s28::Node::Config config;
    s28::Dir d(config, ".", nullptr);
    d.build(config);

    s28::collector::Records records;
    s28::walk(&d, boost::bind(&s28::collector::find, _1, boost::ref(records)));

    s28::Progress progress;


    s28::collector::stat(records, progress);
    s28::collector::hash(records, progress);

    s28::collector::group_duplicates(records, progress);

    for (auto &recit: records) {
        auto &rec = recit.second;
        if (rec.hash.empty()) continue;

        std::cout << rec.node->get_path() << " " << (rec.hash);
        int cnt = 0;
        s28::collector::Record *next = rec.repre;
        while (next) {
            ++cnt;
            next = next->next;
        }

        if (rec.repre) {
            std::cout << " = " << rec.repre->node->get_path() ;

        }

        std::cout << std::endl;
//        std::cout << " " << rec.inode << " " << cnt << std::endl;
    }
/*

    s28::Collector collector;

    s28::walk(&d, boost::bind(&s28::Collector::index, &collector, _1));
    s28::walk(&d, boost::bind(&s28::Collector::hash, &collector, _1));
    s28::walk(&d, boost::bind(&s28::Collector::stat, &collector, _1));

    collector.find_duplicates();
    collector.find_links();
*/
 //   s28::walk(&d, boost::bind(&s28::Collector::dump, &collector, _1));

//    d.traverse(collector);

    return 0;
}

