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

#include "node.h"
#include "dir.h"

#include "escape.h"
#include "error.h"
#include "hash.h"
#include "file.h"
#include "parser.h"

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

class BaseRecord : public boost::noncopyable {
    public:
        const Node *node = nullptr;
        ino_t inode = 0;
        int brackets = 0;
};

class Record : public BaseRecord  {
public:
    std::string hash;
    Record *repre = nullptr;
    Record *next = nullptr;
};

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

std::string read_escaped_string(parser::Parslet &p) {
    parser::ltrim(p);
    parser::Parslet orig = p;

    if (*p == '\'') {
        p.skip();
        while (*p != '\'') {
            if (p.next() == '\\') {
                if (p.next() == 'x') {
                    p.skip();
                }
                p.skip();
                continue;
            }
        }
        p.skip();
    } else {
        while (!isspace(*p)) {
            if (p.next() == '\\') {
                if (p.next() == 'x') {
                    p.skip();
                }
                p.skip();
                continue;
            }
        }
    }
    Escaper es;
    return es.unescape(parser::Parslet(orig.begin(), p.begin()).str());
}


namespace parser {
std::string number(parser::Parslet &p) {
    std::string n;
    while (isdigit(*p)) {
        n += p.next();
    }
    return n;
}
}

class RenameParser {
public:
    void read_dir_content(parser::Parslet &p, const std::string &prefix) {
        for (;;) {
            parser::ltrim(p);
            if (*p == '}') {
                p.skip();
                return;
            }
            std::string filename = read_escaped_string(p);
            std::string path = prefix + "/" + filename;
            //        std::cout << "[" << prefix << "/" << filename << "]" << std::endl;
            parser::ltrim(p);
            if (*p == '{') {
                p.skip();
                read_dir_content(p, prefix + "/" + filename);
            }
            if (*p == '}') {
                p.skip();
                return;
            }
            if (*p == '#') {
                p.skip();
                for(;;) {
                    std::set<ino_t> inodes;
                    while(isdigit(*p)) {
                        inodes.insert(boost::lexical_cast<ino_t>(parser::number(p)));
                    }
                    if (*p != '|') break;
                    p.skip();
                }



                while (*p != '\n') p.skip();
            }
        }
    }

    void parse(const std::string &inputfile) {
        std::ifstream is (inputfile, std::ifstream::binary);
        std::string str((std::istreambuf_iterator<char>(is)),
                std::istreambuf_iterator<char>());


        parser::Parslet p(str);
        std::string filename = read_escaped_string(p);
        parser::ltrim(p);
        if (*p == '{') {
            p.skip();
            read_dir_content(p, filename);
        }
    }

};

} // namespace s28

std::string tabs(int n) {
    std::string rv;

    while(n > 0) {
        rv += "  ";
        n--;
    }
    return rv;
}

int main() {
#if 1
    s28::Node::Config config;
    s28::Dir d(config, ".", nullptr);
    d.build(config);

    s28::collector::BaseRecords records;
    s28::collector::BaseRecordsBuilder rb(records);
    d.traverse(rb);


    s28::Progress progress;
    s28::collector::stat(records, progress);

    std::map<ino_t, s28::collector::BaseRecord *> inomap;
    for (auto &r: records) {
        std::cout << r->node->get_path() << std::endl;
        inomap[r->inode] = r.get();
    }

    s28::RenameParser rp;
    rp.parse("a");

    return 0;
#else
    s28::Node::Config config;
    s28::Dir d(config, ".", nullptr);
    d.build(config);

    s28::collector::Records records;
    s28::collector::RecordsBuilder rb(records);
    d.traverse(rb);

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
#endif
}

