#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#include <map>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <openssl/sha.h>
#include "escape.h"
#include "error.h"

namespace s28 {
class FileDescriptorGuard {
public:
    FileDescriptorGuard(int fd) : fd(fd) {}
    ~FileDescriptorGuard() {
        if (fd >= 0) ::close(fd);
    }
    int fd;
};

class NodeContext;
class Node {
public:
    Node(const std::string &path, NodeContext &context) :
        path(path), context(context)
    {}

    virtual void build() = 0;

protected:
    const std::string path;
    NodeContext &context;
};


class Dir : public Node {
public:
    using Node::Node;

    void build() override;

private:
    std::vector<std::unique_ptr<Node>> children;
};

class File : public Node {
public:
    using Node::Node;
    void build() override;

    bool is_hashed() const { return hashed; }
    std::string get_hash() {
        return std::string((char *)hash, SHA256_DIGEST_LENGTH);
    }
private:
    unsigned char hash[SHA256_DIGEST_LENGTH];
    bool hashed = false;
};

class NodeContext {
public:
    uint32_t hash_id(unsigned char *hash) { return 0; }

private:
    std::map<std::string, int> file_ids;
};




void File::build() {
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
    } while(len != sizeof(buf));
    SHA256_Final(hash, &sha256);
    hashed = true;
}


void Dir::build() {
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
                std::unique_ptr<Dir> node(new Dir(path + "/" + name, context));
                children.push_back(std::move(node));
                break;
            }
            case DT_REG: {
                std::unique_ptr<File> node(new File(path + "/" + name, context));
                children.push_back(std::move(node));


                break;
            }
        }
    }

    for(auto &dir: children) {
        dir->build();
    }
}


} // namespace s28


int main() {
    s28::init_escaping();
    return 0;
}

