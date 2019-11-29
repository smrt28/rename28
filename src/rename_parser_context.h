#ifndef RENAME_PARSER_CONTEXT_H
#define RENAME_PARSER_CONTEXT_H
#include <set>
#include <map>
#include "hash.h"

namespace s28 {

typedef std::vector<std::string> DirChain;

class DirChainSet {
    public:
        typedef hash128_t Stamp;

        size_t insert(const DirChain &chain) {
            return hashes[hash_dirchain(chain)]++;
        }

        size_t count(const DirChain &chain) const {
            auto it = hashes.find(hash_dirchain(chain));
            if (it == hashes.end()) return 0;
            return it->second;
        }

    private:
        std::map<Stamp, size_t> hashes;
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
    GlobalRenameContext &global_context;
    size_t dirorder = 0;
    size_t fileorder = 0;
    size_t insert_node(const DirChain &chain) {
        return global_context.nodes.insert(chain);
    }
    size_t nodes_count(const DirChain &chain) const {
        return global_context.nodes.count(chain);
    }
};

}

#endif /* RENAME_PARSER_CONTEXT_H */
