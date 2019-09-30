#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>


#include <set>
#include <map>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <openssl/sha.h>

#include <boost/bind.hpp>

#include "escape.h"
#include "error.h"
#include "hash.h"

namespace s28 {

class Node;

class Traverse {
public:
    virtual void walk(const Node *) {};
};


class Node {
public:
    class Config {
    public:
        uint32_t get_next_id() {
            return ++next_id;
        }
    private:
        uint32_t next_id = 0;
    };

    Node(Config &config, const std::string &path) :
        path(path), id(config.get_next_id())
    {}

    virtual ~Node() {}

    virtual void build(Config &) = 0;
    virtual void traverse(Traverse &t) const = 0;

    std::string get_path() const { return path; }

    uint32_t get_id() const { return id; }
protected:
    const std::string path;
    uint32_t id;
};


class Dir : public Node {
public:
    using Node::Node;

    void build(Config &) override;
    void traverse(Traverse &t) const override;

private:
    std::vector<std::unique_ptr<Node>> children;
};



void Dir::traverse(Traverse &t) const {
    t.walk(this);
    for (auto &a: children) {
        a->traverse(t);
    }
}


class File : public Node {
public:
    using Node::Node;
    void build(Config &) override;
    void traverse(Traverse &t) const override;
};

void File::traverse(Traverse &t) const {
    t.walk(this);
}




void File::build(Config &) {
    /*
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    int fd = open(path.c_str(), O_RDONLY);
    if (fd == -1) RAISE_ERROR("open for reading; file=" << path);
    FileDescriptorGuard guard(fd);

    char buf[4096];
    ssize_t len;

    do {
        len = read(fd, buf, sizeof(buf));
        if (len < 0)
            RAISE_ERROR("error while reading; file=" << path);

        SHA256_Update(&sha256, buf, len);
    } while(len == sizeof(buf));
    SHA256_Final(hash, &sha256);
    hashed = true;
    */
}


/*
class Collector : public Traverse {
public:
    void walk(const Node *) override;
    std::map<std::string, uint32_t> unique;
};

void Collector::walk(const Node *node) {
    const File *f = dynamic_cast<const File *>(node);
    if (f) {
        try {
            std::string h = hash_file_short(node->get_path());
            auto it = unique.find(h);
            if (it == unique.end()) {
                unique[h] = unique.size();
            }

            std::cout << unique[h] << "::" << escape(node->get_path()) << std::endl;
        } catch(...) {}
    }
}
*/

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


void Dir::build(Config &config) {
    DIR *dp;
    struct dirent *entry;

    if((dp = opendir(path.c_str())) == NULL) {
        return;
    }

    while(( entry = readdir(dp)) != NULL) {
        std::string name = entry->d_name;
        if (name == ".." || name == ".") continue;

        switch(entry->d_type) {
            case DT_DIR: {
                std::unique_ptr<Dir> node(new Dir(config, path + "/" + name));
                children.push_back(std::move(node));
                break;
            }
            case DT_REG: {
                std::unique_ptr<File> node(new File(config, path + "/" + name));
                children.push_back(std::move(node));
                break;
            }
        }
    }

    for(auto &dir: children) {
        dir->build(config);
    }
}

class Collector {
    struct Record {
        std::string hash;
        uint32_t order = 0;
        const Node *node = nullptr;
    };
public:

    void index(const Node *n) {
        records[n->get_id()].node = n;
    }

    void hash(const Node *n) {
        const File *f = dynamic_cast<const File *>(n);
        if (!f) return;

        records[f->get_id()].hash = hash_file_short(f->get_path());
    }

    void find_duplicates() {
        std::map<std::string, uint32_t> dups;
        {
            std::set<std::string> all;
            for (auto const &record: records) {
                if (all.count(record.second.hash)) {
                    auto it = dups.find(record.second.hash);
                    if (it == dups.end()) {
                        dups[record.second.hash] = 2;
                    } else {
                        it->second ++;
                    }
                } else {
                    all.insert(record.second.hash);
                }
            }
        }

        for (auto &record: records) {
            auto it = dups.find(record.second.hash);
            if (it == dups.end()) continue;
            record.second.order = it->second;
            it->second--;
        }
    }

    void dump() {
        for (auto &r: records) {
            if (dynamic_cast<const File *>(r.second.node)) {
                std::cout << r.second.hash << "-" << r.second.order << ": " << r.second.node->get_path() << std::endl;
            }
        }
    }


private:
    std::map<uint32_t, Record> records;
};


} // namespace s28


int main() {
    s28::init_escaping();

    s28::Node::Config config;
    s28::Dir d(config, ".");
    d.build(config);
    s28::Collector collector;



    s28::walk(&d, boost::bind(&s28::Collector::index, &collector, _1));
    s28::walk(&d, boost::bind(&s28::Collector::hash, &collector, _1));

    collector.find_duplicates();
    collector.dump();

    return 0;
}

