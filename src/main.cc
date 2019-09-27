#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <map>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <openssl/sha.h>

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
    };

    Node(const std::string &path) :
        path(path)
    {}

    virtual ~Node() {}

    virtual void build(const Config &) = 0;
    virtual void traverse(Traverse &t) = 0;

    std::string get_path() const { return path; }
protected:
    const std::string path;
};


class Dir : public Node {
public:
    using Node::Node;

    void build(const Config &) override;
    void traverse(Traverse &t) override;

private:
    std::vector<std::unique_ptr<Node>> children;
};



void Dir::traverse(Traverse &t) {
    t.walk(this);
    for (auto &a: children) {
        a->traverse(t);
    }
}


class File : public Node {
public:
    using Node::Node;
    void build(const Config &) override;
    void traverse(Traverse &t) override;
};

void File::traverse(Traverse &t) {
    t.walk(this);
}




void File::build(const Config &) {
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

void Dir::build(const Config &config) {
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
                std::unique_ptr<Dir> node(new Dir(path + "/" + name));
                children.push_back(std::move(node));
                break;
            }
            case DT_REG: {
                std::unique_ptr<File> node(new File(path + "/" + name));
                children.push_back(std::move(node));
                break;
            }
        }
    }

    for(auto &dir: children) {
        dir->build(config);
    }
}


} // namespace s28


int main() {
    s28::init_escaping();
    s28::Dir d(".");
    s28::Node::Config config;
    d.build(config);

    s28::Collector collector;
    d.traverse(collector);

    return 0;
}

