#include "file.h"
#include "dir.h"

namespace s28 {

void File::traverse(Traverse &t) const {
    t.walk(this);
}

void File::build(Config &) {
}

std::string File::get_path() const {
    return parent->get_path() + name;
}

} // namespace s28
