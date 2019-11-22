#ifndef RENAME_PARSER_CONTEXT_H
#define RENAME_PARSER_CONTEXT_H
#include <set>

namespace s28 {

class GlobalRenameContext {
public:
    std::set<std::string> files;
    std::set<std::string> dirs;

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
