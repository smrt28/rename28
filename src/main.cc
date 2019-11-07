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
#include <fstream>

#include <openssl/sha.h>
#include <boost/algorithm/string/join.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <boost/program_options.hpp>

#include "node.h"
#include "dir.h"

#include "escape.h"
#include "error.h"
#include "hash.h"
#include "file.h"
#include "parser.h"
#include "utils.h"
#include "rename_parser.h"
#include "record.h"

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
    virtual void tick(size_t n, size_t total) {}
};

template<typename CB>
void walk(const Node *node, CB cb) {
    aux::Collector<CB> collector(node, cb);
    node->traverse(collector);
}



namespace collector {


typedef std::vector<std::unique_ptr<Record>> Records;
typedef std::vector<std::unique_ptr<BaseRecord>> BaseRecords;


void find(const Node *n, Records &records) {
    std::unique_ptr<Record> rec(new Record());
    rec->node = n;
    records.push_back(std::move(rec));
}


template<typename RECORDS>
void stat(RECORDS &records, Progress &progress) {
    size_t cnt = 0;
    for (auto &rec: records) {
        progress.tick(++cnt, records.size());
        struct stat stt;
        std::string path = rec->node->get_path();
        if (::stat(path.c_str(), &stt) == -1) {
            RAISE_ERROR("stat failed; file=" << rec->node->get_path());
        }
        rec->inode = stt.st_ino;
    }
}

void hash(Records &records, Progress &progress) {
    size_t cnt = 0;
    for (auto &rec: records) {
        progress.tick(++cnt, records.size());
        const Node *n = rec->node;
        const File *f = dynamic_cast<const File *>(n);
        if (!f) continue;
        rec->hash = hash_file_short(f->get_path());
    }
}


void group_duplicates(Records &records, Progress &progress) {
    size_t cnt = 0;
    std::map<std::string, s28::collector::Record *> uniq;
    for (auto &rec: records) {
        progress.tick(++cnt, records.size());
        if (rec->hash.empty()) continue;
        auto it = uniq.find(rec->hash);
        if (it == uniq.end()) {
            uniq[rec->hash] = rec.get();
        } else {
            s28::collector::Record *repre = it->second->repre;
            if (repre) {
                rec->repre = repre;
                rec->next = repre->next;
            } else {
                repre = it->second;
                repre->repre = rec->repre = repre;
            }
            repre->next = rec.get();
        }
    }
}


template<typename REC>
class RecordsBuilderImpl : public Traverse {
public:
    typedef std::vector<std::unique_ptr<REC>> Records;

    RecordsBuilderImpl(Records &records) : records(records) {}
    void walk(const Node *node) override {
        std::unique_ptr<REC> rec(new REC());
        rec->node = node;
        records.push_back(std::move(rec));
    }

    void on_dir_end(const Node *n) override {
        records.back()->brackets ++;
    }

private:
    Records &records;
};


typedef RecordsBuilderImpl<Record> RecordsBuilder;
typedef RecordsBuilderImpl<BaseRecord> BaseRecordsBuilder;

}



} // namespace s28

std::string tabs(int n) {
    std::string rv;

    while(n > 0) {
        rv += "  ";
        n--;
    }
    return rv;
}



int search_rename_repo() {
    s28::Node::Config config;
    s28::Dir d(config, ".renameRepo", nullptr);
    d.build(config);

    s28::collector::Records records;
    s28::collector::RecordsBuilder rb(records);
    d.traverse_children(rb);

    s28::Progress progress;
    s28::collector::stat(records, progress);
    s28::collector::hash(records, progress);
    s28::collector::group_duplicates(records, progress);

    s28::Escaper es;

    int dep = 0;
    for (auto &rec: records) {
        auto * node = rec->node;

        if (node->is<s28::Dir>()) {
            const s28::Dir *dir = dynamic_cast<const s28::Dir *>(node);
            if (dir->get_children().empty()) {
                std::cout << tabs(dep) << es.escape(node->get_name()) << " {}";
            } else {
                std::cout << tabs(dep) << es.escape(node->get_name()) << " {";
                dep += 1;
            }
        } else {
            std::cout << tabs(dep) << es.escape(node->get_name()) << " #" << rec->inode;
            auto d = rec->repre;
            if (d) {
                while(d) {
                    if (d->inode != rec->inode) {
                        std::cout << "|" << d->inode;
                    }
                    d = d->next;
                }
                std::cout << " !Dup";
            }
        }

        std::cout << std::endl;

        for (int i = 0; i < rec->brackets; ++i) {
            dep --;
            std::cout << tabs(dep) << "}" << std::endl;
        }
    }
    return 0;
}

int apply_rename() {
    s28::Node::Config config;
    s28::Dir d(config, ".renameRepo", nullptr);
    d.build(config);

    s28::collector::BaseRecords records;
    s28::collector::BaseRecordsBuilder rb(records);
    d.traverse(rb);


    s28::Progress progress;
    s28::collector::stat(records, progress);

    s28::RenameParser::InodeMap inomap;
    for (auto &r: records) {
        inomap[r->inode] = r.get();
    }

    s28::RenameParser rp(inomap);
    rp.parse("a");

    return 0;
}

int main(int argc, char **argv) {
    using namespace boost::program_options;
    options_description desc{"Options"};
    desc.add_options()
        ("help,h", "Help screen")
        ("init,i", "initialize repo")
        ("apply,a", "apply .rename")
        ;

    variables_map vm;
    store(parse_command_line(argc, argv, desc), vm);
    notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 0;

    }

    if (vm.count("init")) {
        search_rename_repo();
        return 0;
    }

    if (vm.count("apply")) {
        apply_rename();
        return 0;

    }
}
