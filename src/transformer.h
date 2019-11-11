#ifndef TRRANSFORMER_H
#define TRRANSFORMER_H
namespace s28 {

class Transformer {
public:
    enum Type {
        DIRNAME, FILENAME
    };
    Transformer(int dep) : dep(dep) {}
    virtual std::string transform(const std::string &filename, Type) = 0;
    const int dep;
};

namespace tformer {
class Numbers : public Transformer {
    using Transformer::Transformer;
    std::string transform(const std::string &filename, Type type) override {
        if (type == DIRNAME) return filename;
        return filename + "_" + std::to_string(++n);
    }
    int n = 0;
};

class Flatten : public Transformer {
    public:
    Flatten(int dep, const std::string &path): Transformer(dep), path(path) {
        std::cerr << "flatten: " << path << std::endl;
    }
    std::string transform(const std::string &filename, Type type) override {
        if (type == FILENAME)
            return filename;
        return path;
    }
    std::string path;
};

}


}

#endif /* TRRANSFORMER_H */
