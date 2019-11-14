#ifndef TRRANSFORMER_H
#define TRRANSFORMER_H

#include "error.h"
namespace s28 {

typedef std::vector<std::string> DirChain;


class DirPathBuilder {
public:
    virtual bool build_dir_path(const DirChain &dirchain, std::string &path) {
        path = boost::algorithm::join(dirchain, "/");
        return true;
    }
};

class FilePathBuilder {
public:
    virtual bool build_file_path(const DirChain &dirchain, std::string &path) {
        path = boost::algorithm::join(dirchain, "/");
        return true;
    }
};

class FileFlattener : public FilePathBuilder {
public:
    FileFlattener(size_t dep, FilePathBuilder *parent) : dep(dep), parent(parent) {}

    bool build_file_path(const DirChain &dirchain, std::string &path) {
        DirChain v(dirchain.begin(), dirchain.begin() + dep);
        v.push_back(dirchain.back());
        return parent->build_file_path(v, path);
    }

    size_t dep;
    FilePathBuilder *parent;
};

class DirFlattener : public DirPathBuilder {
public:
    DirFlattener(size_t dep, DirPathBuilder *parent) : dep(dep), parent(parent) {}

    bool build_dir_path(const DirChain &dirchain, std::string &path) {
        if (dirchain.size() > dep) return false;
        return parent->build_dir_path(dirchain, path);
    }

    size_t dep;
    DirPathBuilder *parent;
};


class FileNumerator : public FilePathBuilder {
public:
    FileNumerator(FilePathBuilder *parent) : parent(parent) {}
    bool build_file_path(const DirChain &dirchain, std::string &path) {
        DirChain v(dirchain.begin(), dirchain.end() - 1);
        v.push_back(dirchain.back() + "-" + std::to_string(++n));
        return parent->build_file_path(v, path);
    }

    FilePathBuilder *parent;
    int n = 0;
};


} // namespace s28

#endif /* TRRANSFORMER_H */
