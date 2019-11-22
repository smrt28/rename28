#ifndef RENAME_PARSER_CONTEXT_H
#define RENAME_PARSER_CONTEXT_H
#include <set>
#include "hash.h"

namespace s28 {

typedef std::vector<std::string> DirChain;

class DirChainSet {
    public:
        void insert(const DirChain &chain) {
            hashes.insert(hash_dirchain(chain));
        }

        size_t count(const DirChain &chain) const {
            return hashes.count(hash_dirchain(chain));
        }

    private:
        std::set<hash128_t> hashes;
};

class GlobalRenameContext {
public:
    DirChainSet nodes;
};

class RenameParserContext {
public:
    RenameParserContext(GlobalRenameContext &global_context) :
        global_context(global_context)
    {}
    const GlobalRenameContext &global_context;
    size_t dirorder = 0;
    size_t fileorder = 0;
};

}

#endif /* RENAME_PARSER_CONTEXT_H */
