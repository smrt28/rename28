#include <sys/types.h>
#include <dirent.h>

#include <memory>

#include "dir.h"
#include "file.h"

namespace s28 {
namespace {

class DirDescriptorGuard {
public:
    DirDescriptorGuard(DIR *dir) : dir(dir) {}
    ~DirDescriptorGuard() {
        if (dir) ::closedir(dir);
    }
    DIR *dir;
};

} // namespace

void Dir::traverse(Traverse &t) const {
    t.walk(this);
    for (auto &a: children) {
        a->traverse(t);
    }
}


void Dir::build(Config &config) {
    DIR *dp = nullptr;
    struct dirent *entry = nullptr;

    if((dp = opendir(path.c_str())) == NULL) {
        return;
    }

    DirDescriptorGuard guard(dp);

    while(( entry = readdir(dp)) != NULL) {
        std::string name = entry->d_name;
        if (name == ".." || name == ".") continue;

        switch(entry->d_type) {
            case DT_DIR: {
                std::unique_ptr<Dir> node(new Dir(config, path + "/" + name, this));
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

} // namespace s28
