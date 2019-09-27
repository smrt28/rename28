#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <sstream>

#include "escape.h"
#include "error.h"

namespace s28 {

class Node {
public:
    class Context {

    };

    Node(const std::string &path, Context &context) : path(path), context(context) {
                std::cout << path << std::endl;
    }

    virtual void walk() = 0;

protected:
    const std::string path;
    Context &context;
};


class Dir : public Node {
public:
    using Node::Node;

    void walk() override;

private:
    std::vector<std::unique_ptr<Node>> children;
};

class File : public Node {
    using Node::Node;

    void walk() override {}
};

void Dir::walk() {
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;

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
        dir->walk();
    }
}


} // namespace s28


int main() {
    //s28::Dir::Context context;
    //s28::Dir(".", context).walk();
    //
    s28::init_escaping();

    std::string x = "a\t\n\013\\123 ::: :_\n\n\n\\\\\"$\09fasdfgwg\xewg";
    x[19] = 0xf5;

    std::cout << "[" << s28::escape(x) << "]" << std::endl;
    std::cout << "[" << s28::escape("Hello world!") << "]" << std::endl;
    std::cout << (s28::unescape(s28::escape(x)) == x) << std::endl;
}

