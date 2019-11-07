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
    if (children.empty()) return;
    t.on_dir_begin(this);
    for (size_t i = 0; i < children.size(); ++i) {
        children[i]->traverse(t);
    }
    t.on_dir_end(this);
}

void Dir::traverse_children(Traverse &t) const {
    if (children.empty()) return;
    for (size_t i = 0; i < children.size(); ++i) {
        children[i]->traverse(t);
    }
}


void Dir::build(Config &config) {
    DIR *dp = nullptr;
    struct dirent *entry = nullptr;

    if((dp = opendir(get_path().c_str())) == NULL) {
        return;
    }

    DirDescriptorGuard guard(dp);

    while(( entry = readdir(dp)) != NULL) {
        std::string dname = entry->d_name;
        if (dname == ".." || dname == ".") continue;

        switch(entry->d_type) {
            case DT_DIR: {
                std::unique_ptr<Dir> node(new Dir(config, dname, this));
                children.push_back(std::move(node));
                break;
            }
            case DT_REG: {
                std::unique_ptr<File> node(new File(config, dname, this));
                children.push_back(std::move(node));
                break;
            }
        }
    }

    for(auto &dir: children) {
        dir->build(config);
    }
}

std::string Dir::get_path() const {
    if (!parent) return name + "/";
    return parent->get_path() + name + "/";
}


} // namespace s28
