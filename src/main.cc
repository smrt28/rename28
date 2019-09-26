#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <vector>
#include <memory>

namespace s28 {

namespace {
    std::string esc_chars[256];
    void init_esc_chars() {
        const char *hex = "0123456789ABCDEF";
        for (int i = 0; i < 256; ++i) {
            if (isprint(i)) {
                char buf[2];
                buf[0] = i;
                buf[1] = 0;
                esc_chars[i] = buf;
            } else {
                esc_chars[i] = "\\x";
                esc_chars[i] += hex[i >> 4];
                esc_chars[i] += hex[i & 0xf];
            }
        }

        esc_chars['\n'] = "\\n";
        esc_chars['\t'] = "\\t";
        esc_chars['\\'] = "\\\\";
    }

    int hex2int(char c) {
        switch(c) {
            case '0': return 0;
            case '1': return 1;
            case '2': return 2;
            case '3': return 3;
            case '4': return 4;
            case '5': return 5;
            case '6': return 6;
            case '7': return 7;
            case '8': return 8;
            case '9': return 9;
            case 'A': return 10;
            case 'a': return 10;
            case 'B': return 11;
            case 'b': return 11;
            case 'C': return 12;
            case 'c': return 12;
            case 'D': return 13;
            case 'd': return 13;
            case 'E': return 14;
            case 'e': return 14;
            case 'F': return 15;
            case 'f': return 15;
        }

        throw 1;
    }
}


std::string escape(const std::string &s) {
    std::string rv;

    for (char c: s) {
        rv += esc_chars[(unsigned char )c];
    }
    return rv;
}

std::string unescape(const std::string &s) {
    std::string rv;
    for (int i = 0; i < s.size(); ++i) {
        if (s[i] == '\\') {
            if (i == s.size() - 1) throw 1;
            switch (s[i + 1]) {
                case 'n':
                    i += 1; rv += "\n";
                    break;
                case 't':
                    i += 1; rv += "\t";
                    break;
                case '\\':
                    i += 1; rv += "\\";
                    break;
                case 'x':
                    if (i + 3 >= s.size()) throw 1;
                    rv += (char)( (hex2int(s[i + 2]) << 4) + hex2int(s[i + 3]));
                    i+=3;
                    break;
            }

        } else {
            rv += s[i];
        }
    }
    return rv;
}


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
    s28::init_esc_chars();

    std::string x = "a\t\n\013\\123 :_\n\n\n\\\\\"$\09fasdfgwg\xewg";
    x[19] = 0xf5;
    std::cout << (s28::unescape(s28::escape(x)) == x) << std::endl;
}

