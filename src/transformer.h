#ifndef TRRANSFORMER_H
#define TRRANSFORMER_H
namespace s28 {

class DupHandler {
public:
    DupHandler(int dep) : dep(dep) {}

    bool check_duplicate(ino_t ino) {
        if (dups.count(ino)) return true;
        dups.insert(ino);
        return false;
    }

    std::set<ino_t> dups;
    const int dep;
};

class Transformer {
public:
    enum Type {
        DIRNAME, FILENAME
    };

    static const uint32_t OK            = 0;


    Transformer(int dep) : dep(dep) {}
    virtual int transform(std::string &path, std::string &filename, Type) { return OK; }
    const int dep;
};

namespace tformer {
class Numbers : public Transformer {
    using Transformer::Transformer;

    int transform(std::string &path, std::string &filename, Type type) override {
        if (type == DIRNAME) return OK;
        if (filename.empty()) {
            filename = std::to_string(++n);
        } else {
            filename = std::to_string(++n) + "_" + filename;
        }

        return OK;
    }

    int n = 0;
};

class Flatten : public Transformer {
public:
    Flatten(int dep, const std::string &path): Transformer(dep), fixedpath(path) {   }


    int transform(std::string &path, std::string &filename, Type type) override {
        path = fixedpath;
        if (type == DIRNAME) {
            filename = "";
            return OK;
        }

        if (usednames.count(filename)) {
            int dups = 0;
            for (;;) {
                dups ++;
                std::string s = filename + "_dup" + std::to_string(dups);
                if (!usednames.count(s)) {
                    filename = s;
                    break;
                }
            }
        }

        usednames.insert(filename);

        return OK;
    }

    std::string fixedpath;
    std::set<std::string> usednames;
};

} // namespace tformer
} // namespace s28

#endif /* TRRANSFORMER_H */
