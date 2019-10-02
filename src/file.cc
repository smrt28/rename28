#include "file.h"

namespace s28 {

void File::traverse(Traverse &t) const {
    t.walk(this);
}

void File::build(Config &) {
}

} // namespace s28
