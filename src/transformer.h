#ifndef TRRANSFORMER_H
#define TRRANSFORMER_H
namespace s28 {

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
        filename = std::to_string(++n) + "_" + filename;
        return OK;
    }

    int n = 0;
};

class Flatten : public Transformer {
public:
    Flatten(int dep, const std::string &path): Transformer(dep), fixedpath(path) {   }


    int transform(std::string &path, std::string &filename, Type type) override {
        path = fixedpath;
        if (type == DIRNAME) filename = "";
        return OK;
    }

    std::string fixedpath;
};

} // namespace tformer
} // namespace s28

#endif /* TRRANSFORMER_H */
