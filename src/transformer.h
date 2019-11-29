#ifndef TRRANSFORMER_H
#define TRRANSFORMER_H
#include <vector>
#include <string>

#include <boost/algorithm/string/join.hpp>
#include "error.h"
#include "pattern.h"
#include "rename_parser_context.h"

namespace s28 {


class PathBuilder {
public:
    enum Result {
        SKIP,
        CHANGED,
        UNCHANGED
    };

    virtual ~PathBuilder() {}
    virtual Result build(const DirChain &dirchain, DirChain &path) {
        return UNCHANGED;
    }

    virtual Result build(const DirChain &dirchain, DirChain &path, const RenameParserContext &ctx, int dups) {
        return build(dirchain, path);
    }
};

class FileFlattener : public PathBuilder {
public:
    FileFlattener(size_t dep) : dep(dep) {}

    Result build(const DirChain &dirchain, DirChain &path) override {
        DirChain v(dirchain.begin(), dirchain.begin() + dep);
        v.push_back(dirchain.back());
        std::swap(v, path);
        return CHANGED;
    }

    size_t dep;
};

class DirFlattener : public PathBuilder {
public:
    DirFlattener(size_t dep) : dep(dep) {}

    Result build(const DirChain &dirchain, DirChain &path) override {
        if (dirchain.size() > dep) return SKIP;
        return UNCHANGED;
    }

    size_t dep;
};


class ApplyPattern : public PathBuilder {
public:
    ApplyPattern(const std::string &pattern) :
        pattern_parser()
    {
        pattern_parser.build(pattern);
    }

    Result build(const DirChain &dirchain, DirChain &path, const RenameParserContext &ctx, int dups) override {
        {
            DirChain v(dirchain.begin(), dirchain.end() - 1);
            v.push_back(pattern_parser.parse(dirchain.back(), ctx, dups));

            if (!ctx.global_context.nodes.count(v)) {
                std::swap(v, path);
                return CHANGED;
            }
        }


        for (int dups = 1; dups < 100; ++dups) {
            DirChain v(dirchain.begin(), dirchain.end() - 1);
            v.push_back(pattern_parser.parse(dirchain.back(), ctx, dups));
            if (!ctx.global_context.nodes.count(v)) {
                std::swap(v, path);
                return CHANGED;
            }
        }

        RAISE_ERROR("too many duplicates");
        return CHANGED;
    }

   PatternParser pattern_parser;
};

class CharFilter : public PathBuilder {
public:
    Result build(const DirChain &dirchain, DirChain &path) override {
        DirChain v(dirchain.begin(), dirchain.end() - 1);

        std::string s = dirchain.back();
        for (size_t i = 0; i < s.size(); ++i) {
            if (isspace(s[i]) || !isprint(s[i])) {
                s[i] = '_';
            }
        }
        v.push_back(s);
        std::swap(v, path);
        return CHANGED;
    }
};


} // namespace s28

#endif /* TRRANSFORMER_H */
